#include <digitizer.hpp>

#include <boost/algorithm/string.hpp>

// logging
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/constant.hpp>

#include <iomanip>   // std::hex

#include <helper.hpp>       // helper functions
#include <caen.hpp>

namespace pt = boost::property_tree;

#define DG_LOG_DEBUG                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::debug)
#define DG_LOG_INFO                                           \
  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::info)
#define DG_LOG_WARN                                             \
  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::warning)
#define DG_LOG_ERROR                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::error)
#define DG_LOG_FATAL                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::fatal)


cadidaq::digitizer::digitizer(std::string name) : name(name), lnk(nullptr), dg(nullptr), reg(nullptr){
  // Register a constant attribute that identifies our digitizer in the logs
  lg.add_attribute("Digitizer", boost::log::attributes::constant<std::string>(name));
}

cadidaq::digitizer::~digitizer(){
  if (dg)
    delete dg;
  if (lnk)
    delete lnk;
  if (reg)
    delete lnk;
}

void cadidaq::digitizer::configure(pt::iptree *node){
  if (dg != nullptr){
    DG_LOG_FATAL << "Digitizer '" << name << "' already configured!";
    return;
  }
  lnk = new cadidaq::connectionSettings(name);
  // parse and store the link settings
  lnk->parse(node);
  lnk->verify();
  // establish connection
  DG_LOG_INFO << "Establishing connection to digitizer '" << name << "': "
                << "' (linkType=" << *lnk->linkType
                << ", linkNum=" << *lnk->linkNum
                << ", ConetNode=" << *lnk->conetNode
                << ", VMEBaseAddress=" << std::hex << std::showbase << *lnk->vmeBaseAddress << ")";
  try{
    dg = caen::Digitizer::open(*lnk->linkType, *lnk->linkNum, *lnk->conetNode, *lnk->vmeBaseAddress);
  }
  catch (caen::Error& e){
    DG_LOG_ERROR << "Caught exception when establishing communication with digitizer " << name << ": " << e.what();
    if (!dg){
      // TODO: more fine-grained error handling, more info on log
      DG_LOG_ERROR << "Please check the physical connection and the connection settings. If using USB link, please make sure that the CAEN USB driver kernel module is installed and loaded, especially after kernel updates (or use DKMS as explained in INSTALL.md).";
      // won't be able to handle this much more gracefully than:
      exit(EXIT_FAILURE);
    }
  }
  // status printout
  DG_LOG_INFO << "Connected to digitzer '" << name << "'" << std::endl
                 << "\t Model:\t\t"           << dg->modelName() << " (numeric model number: " << dg->modelNo() << ")" << std::endl
                 << "\t NChannels:\t"         << dg->channels() << " (in " << dg->groups() << " groups)" << std::endl
                 << "\t ADC bits:\t"          << dg->ADCbits() << std::endl
                 << "\t license:\t"           << dg->license() << std::endl
                 << "\t Form factor:\t"       << dg->formFactor() << std::endl
                 << "\t Family code:\t"       << dg->familyCode() << std::endl
                 << "\t Serial number:\t"     << dg->serialNumber() << std::endl
                 << "\t ROC FW rel.:\t"       << dg->ROCfirmwareRel() << std::endl
                 << "\t AMC FW rel.:\t"       << dg->AMCfirmwareRel() << ", uses DPP FW: " << (dg->hasDppFw() ? "yes" : "no") << std::endl
                 << "\t PCB rev.:\t"          << dg->PCBrevision() << std::endl;

  reg = new cadidaq::registerSettings(name, dg->channels());
  reg->parse(node);
  reg->verify();
  // call our own verification routine to check model-dependent options
  verifySettings();
  // now program the settings
  programSettings(comDirection::WRITING);
  /* Loop over all sub sections and keys that remained after parsing */
  for (auto& key : *node){
    DG_LOG_WARN << "Unknown setting in section " << name << " ignored: \t" << key.first << " = " << key.second.get_value<std::string>();
  }

}

pt::iptree* cadidaq::digitizer::retrieveConfig(){
  if (dg == nullptr){
    DG_LOG_FATAL << "Digitizer '" << name << "' not yet (properly) configured!";
    return nullptr;
  }
  // read the settings back from the device
  programSettings(comDirection::READING);
  // dump settings into a ptree
  pt::iptree *node = lnk->createPTree();
  reg->fillPTree(node);
  return node;
}

//
// programming configuration into digitizer
//



template <typename T>
void cadidaq::digitizer::programWrapper(void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), boost::optional<T> &value, comDirection direction){
  try{
    if (direction == comDirection::WRITING){
      // WRITING
      if (value)
        (dg->*write)(*value);
    } else {
      // READING
      value = (dg->*read)();
    }
  }
  catch (caen::Error& e){
    // TODO: more fine-grained error handling, more info on log
    DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
    if (direction == comDirection::WRITING)
      DG_LOG_ERROR << "\t Calling " << e.where() << " with argument '" << std::to_string(*value) << "' caused exception: " << e.what();
    else
      DG_LOG_ERROR << "\t Calling " << e.where() << " caused exception: " << e.what();
    // setting assumed to be invalid regardless whether we read or write it:
    value = boost::none;
  }
}

template <typename T, typename C>
void cadidaq::digitizer::programWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), C channel, boost::optional<T> &value, comDirection direction){
  try{
    if (direction == comDirection::WRITING){
      // WRITING
      if (value)
        (dg->*write)(channel, *value);
    } else {
      // READING
      value = (dg->*read)(channel);
    }
  }
  catch (caen::Error& e){
    // TODO: more fine-grained error handling, more info on log
    DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
    if (direction == comDirection::WRITING)
      DG_LOG_ERROR << "\t Calling " << e.where() << " for channel/group " << channel << " with argument '" << std::to_string(*value) << "' caused exception: " << e.what();
    else
      DG_LOG_ERROR << "\t Calling " << e.where() << " for channel/group " << channel << " caused exception: " << e.what();
    // setting assumed to be invalid regardless whether we read or write it:
    value = boost::none;
  }
}

template <typename T1, typename T2>
void cadidaq::digitizer::programWrapper(void (caen::Digitizer::*write)(T1, T2), void (caen::Digitizer::*read)(T1&, T2&), boost::optional<T1> &value1, boost::optional<T2> &value2, comDirection direction){
  try{
    if (direction == comDirection::WRITING){
      // WRITING
      if (value1 && value2)
        (dg->*write)(*value1, *value2);
    } else {
      // READING
      T1 v1;
      T2 v2;
      (dg->*read)(v1, v2);
      value1 = v1;
      value2 = v2;
    }
  }
  catch (caen::Error& e){
    // TODO: more fine-grained error handling, more info on log
    DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
    if (direction == comDirection::WRITING)
      DG_LOG_ERROR << "\t Calling " << e.where() << " with arguments '" << std::to_string(*value1) << "', '" << std::to_string(*value2) << "' caused exception: " << e.what();
    else
      DG_LOG_ERROR << "\t Calling " << e.where() << " caused exception: " << e.what();
    // setting assumed to be invalid regardless whether we read or write it:
    value1 = boost::none;
    value2 = boost::none;
  }
}

void cadidaq::digitizer::programMaskWrapper(void (caen::Digitizer::*write)(uint32_t), uint32_t (caen::Digitizer::*read)(), cadidaq::settingsBase::optionVector<bool> &vec, comDirection direction){
  boost::optional<uint32_t> mask = 0;
  // derive the mask in case we are writing it
  if (direction == comDirection::WRITING){
    // check if the setting has been configured at all
    if (countSet(vec.first) == 0)
      return; // keep the default
    mask = vec2Mask(vec.first, dg->groups());
    // verify that channel vector -> group mask conversion is consistent and the same as channel -> channel mask, else warn about misconfiguration
    if (vec2Mask(vec.first, 1, dg->channelsPerGroup()) != vec2Mask(vec.first, 1, 1)){
      DG_LOG_WARN << "Channel mask cannot be exactly mapped to groups of the device '"<< dg->modelName() << "' for setting '" << vec.second << "'. Using instead group mask of " << mask;
    }
  }
  programWrapper(write, read, mask, direction);
  // if reading: now store the retrieved mask it in the vector
  if (direction == comDirection::READING)
    mask2Vec(mask, vec.first, dg->groups());
}


template <typename T, typename C>
void cadidaq::digitizer::programLoopWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), cadidaq::settingsBase::optionVector<T> &vec, comDirection direction, bool ignoreGroups = false){
  int ngroups = dg->groups();
  // if groups are to be ignored
  if (ignoreGroups)
    ngroups = 1;
  // verify that the vector can be put into group structure of the device (if channels are grouped)
  if (ngroups>1){
    for (int i = 0; i<ngroups; i++){
      if (!allValuesSame(vec.first,i*dg->channelsPerGroup(), (i+1)*dg->channelsPerGroup()))
        DG_LOG_WARN << "The channels in the range " << i*dg->channelsPerGroup() << " and " << (i+1)*dg->channelsPerGroup() << " for '" << vec.second << "' are set to different values -> cannot consistently convert to groups supported by the device!";
    }
  }
  // loop over vector's entries and READ/WRITE values from/to digitzer
  for (auto it = vec.first.begin(); it != vec.first.end(); ++it) {
    auto channel = std::distance(vec.first.begin(), it);
    if (direction == comDirection::WRITING){
      if (!*it)
        continue; // skip and leave default
    }
    // map channel number to group index (or set to one if NGroups==1)
    C group = channel/ngroups;
    if (direction == comDirection::READING && channel%ngroups != 0)
      continue; // only read once per group

    // perform the call to the digitizer
    programWrapper(write, read, group, *it, direction);
    if (direction == comDirection::READING){
      if (ngroups > 1){
        // set the other values in the group
        for (int i = group*dg->channelsPerGroup(); i<(group+1)*dg->channelsPerGroup(); i++){
          vec.first.at(i) = *it;
        }
      }
    }
  }
}

/** Implements checks on the configuration options.
    This should take into account all 'Note:' parts of the CAEN digitizer library documentation for the supported models/FW versions. */
void cadidaq::digitizer::verifySettings(){
  // TODO: Trigger Polarity: channel parameter is unused (i.e. the setting is common to all channels) for those digitizers that do not support the individual trigger polarity setting. Please refer to the Registers Description document of the relevant board for check
  // TODO: Trigger Polarity: not for DPP FW
  // TODO: ChannelTriggerThreshold not for DPP FW (inform about alternative setting)
  // TODO: MaxNumEventsBLT: if using DPP-PHA, DPP-PSD or DPP-CI firmware, you have to refer to the SetDPPEventAggregation function.
  // TOOD: options specific to one model should not be set if we are using one without that function
}

/** Implements model/FW-specific settings verification and the calls mapping read/write methods from/to the digitizer and the corresponding the settings.

 */
void cadidaq::digitizer::programSettings(comDirection direction){

  /* data readout */
  if (!dg->hasDppFw()){
    // maxNumEventsBLT only for non-DPP FW, DPP uses SetDPPEventAggregation
    programWrapper(&caen::Digitizer::setMaxNumEventsBLT, &caen::Digitizer::getMaxNumEventsBLT, reg->maxNumEventsBLT.first, direction);
  }

  /* trigger */
  programWrapper(&caen::Digitizer::setSWTriggerMode, &caen::Digitizer::getSWTriggerMode, reg->swTriggerMode.first, direction);
  programWrapper(&caen::Digitizer::setExternalTriggerMode, &caen::Digitizer::getExternalTriggerMode, reg->externalTriggerMode.first, direction);
  programWrapper(&caen::Digitizer::setIOlevel, &caen::Digitizer::getIOlevel, reg->ioLevel.first, direction);
  programWrapper(&caen::Digitizer::setRunSynchronizationMode, &caen::Digitizer::getRunSynchronizationMode, reg->runSyncMode.first, direction);
  programWrapper(&caen::Digitizer::setOutputSignalMode, &caen::Digitizer::getOutputSignalMode, reg->outSignalMode.first, direction);
  if (!dg->hasDppFw()){
    // Standard FW only

    // NOTE: Trigger Polarity: channel parameter is unused (i.e. the setting is common to all channels) for those digitizers that do not support the individual trigger polarity setting. Please refer to the Registers Description document of the relevant board for check
    programLoopWrapper(&caen::Digitizer::setTriggerPolarity, &caen::Digitizer::getTriggerPolarity, reg->chTriggerPolarity, direction);
    // settings different to devices with grouped/ungrouped channels
    if (dg->groups() == 1){
      // no grouped channels
      programLoopWrapper(&caen::Digitizer::setChannelTriggerThreshold, &caen::Digitizer::getChannelTriggerThreshold, reg->chTriggerThreshold, direction);
    } else {
      // channels are grouped
      programLoopWrapper(&caen::Digitizer::setGroupTriggerThreshold, &caen::Digitizer::getGroupTriggerThreshold, reg->chTriggerThreshold, direction);
    }
  } else {
    // DPP FW only
    
  } // hasDPP
  // Standard FW and DPP, either grouped or non-grouped channels:
  if (dg->groups() == 1){
    // no grouped channels
    // TODO: find out whether or not to call this with DPP FW present! Documentation not 100% clear on that.. (use DPPParams.selft = ... instead?)
    programLoopWrapper(&caen::Digitizer::setChannelSelfTrigger, &caen::Digitizer::getChannelSelfTrigger, reg->chSelfTrigger, direction);
  } else {
    // channels are grouped
    // TODO: find out whether or not to call this with DPP FW present! Documentation not 100% clear on that.. (use DPPParams.selft = ... instead?)
    programLoopWrapper(&caen::Digitizer::setGroupSelfTrigger, &caen::Digitizer::getGroupSelfTrigger, reg->chSelfTrigger, direction);
  }

  /* acquisition */
  // setRecordLength requires subsequent call to SetPostTriggerSize
  programWrapper(&caen::Digitizer::setAcquisitionMode, &caen::Digitizer::getAcquisitionMode, reg->acquisitionMode.first, direction);
  programWrapper(&caen::Digitizer::setRecordLength, &caen::Digitizer::getRecordLength, reg->recordLength.first, direction);
  programWrapper(&caen::Digitizer::setPostTriggerSize, &caen::Digitizer::getPostTriggerSize, reg->postTriggerSize.first, direction);
  if (dg->groups() == 1){
    // no grouped channels
    programMaskWrapper(&caen::Digitizer::setChannelEnableMask, &caen::Digitizer::getChannelEnableMask, reg->chEnable, direction);
    programLoopWrapper(&caen::Digitizer::setChannelDCOffset, &caen::Digitizer::getChannelDCOffset, reg->chDCOffset, direction);
  } else {
    // channels are grouped
    programMaskWrapper(&caen::Digitizer::setGroupEnableMask, &caen::Digitizer::getGroupEnableMask, reg->chEnable, direction);
    // NOTE: GroupDCOffset: from AMC FPGA firmware release 0.10 on, it is possible to apply an 8-bit positive digital offset individually to each channel inside a group of the x740 digitizer to finely correct the baseline mismatch. This function is not supported by the CAENdigitizer library, but the user can refer the registers documentation.
    programLoopWrapper(&caen::Digitizer::setGroupDCOffset, &caen::Digitizer::getGroupDCOffset, reg->chDCOffset, direction);
  }
  // X751-family specific settings
  if (dg->is751Family()){
    programWrapper(&caen::Digitizer::setDESMode, &caen::Digitizer::getDESMode, reg->desMode.first, direction);
  }

  // DPP - FW
  if (dg->hasDppFw()){
    // NOTE: loop wrapper is called with ignoreGroups = true as the DPP options are set channel-by-channel in contrast to the non-DPP channel options
    if (dg->isDppCiFw()){
      // DPP-CI only supports ch= -1 (different channels must have the same pre-trigger)
      if (!allValuesSame(reg->dppPreTriggerSize.first)){
        DG_LOG_WARN << "Firmware only supports same pre-trigger for all channels but " << reg->dppPreTriggerSize.second << " not set to same value for all channels. Will apply value given for first channel to all.";
      }
      programWrapper(&caen::Digitizer::setDPPPreTriggerSize, &caen::Digitizer::getDPPPreTriggerSize, -1, reg->dppPreTriggerSize.first.at(0), direction);
      // set other elements in the vector to same value for consistency
      std::fill(reg->dppPreTriggerSize.first.begin(), reg->dppPreTriggerSize.first.end(), reg->dppPreTriggerSize.first.at(0));
    } else {
      programLoopWrapper(&caen::Digitizer::setDPPPreTriggerSize, &caen::Digitizer::getDPPPreTriggerSize, reg->dppPreTriggerSize, direction, true);
    }
    programLoopWrapper(&caen::Digitizer::setChannelPulsePolarity, &caen::Digitizer::getChannelPulsePolarity, reg->dppChPulsePolarity, direction, true);
    programWrapper(&caen::Digitizer::setDPPAcquisitionMode, &caen::Digitizer::getDPPAcquisitionMode, reg->dppAcqMode.first, reg->dppAcqModeParam.first, direction);
    programWrapper(&caen::Digitizer::setDPPTriggerMode, &caen::Digitizer::getDPPTriggerMode, reg->dppTriggermode.first, direction);
  }

  /* program address-value pairs configured individually */
  for (auto r:reg->registerValues){
    try{
      dg->writeRegister(r.first, r.second);
    }
    catch (caen::Error& e){
      DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
      DG_LOG_ERROR << "\t Calling " << e.where() << " for address '" << hex2str(r.first) << "' and value '" <<  hex2str(r.second) << "' caused exception: " << e.what();
    }
  }
}
