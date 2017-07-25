#include <settings.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>   // std::hex
#include <stdexcept> // exceptions
#include <iterator>  // distance

// BOOST
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> // boost::starts_with
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
// logging
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/constant.hpp>

#include <boolTranslator.hpp> // changes 'bool' interpretation in ptrees
#include <hexTranslator.hpp> // changes 'int' interpretation in ptrees to allow reading hex strings

#include <CaenEnum2str.hpp> // generated by CMake in build directory

#define CFG_LOG_DEBUG                                           \
  BOOST_LOG_CHANNEL_SEV(lg, "cfg", boost::log::trivial::debug)
#define CFG_LOG_INFO                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "cfg", boost::log::trivial::info)
#define CFG_LOG_WARN                                              \
  BOOST_LOG_CHANNEL_SEV(lg, "cfg", boost::log::trivial::warning)
#define CFG_LOG_ERROR                                           \
  BOOST_LOG_CHANNEL_SEV(lg, "cfg", boost::log::trivial::error)
#define CFG_LOG_FATAL                                           \
  BOOST_LOG_CHANNEL_SEV(lg, "cfg", boost::log::trivial::fatal)

//
// Helper functions
//

/* expandRange(): splits a comma-separated list of values and ranges into
   individual values, returns these as vector */
  std::vector<int> expandRange(std::string range){
// clean input of spaces
    boost::erase_all(range," ");
    // helper vars
    std::vector<std::string> strs,r;
    std::vector<int> v;
    int low,high,i;
    // split string
    boost::split(strs,range,boost::is_any_of(","));
    // expand values
    for (auto it:strs){
      boost::split(r,it,boost::is_any_of("-"));
      auto x = r.begin();
      low = high =boost::lexical_cast<int>(r[0]);
      x++;
      if(x!=r.end())
        high = boost::lexical_cast<int>(r[1]);
      for(i=low;i<=high;++i)
        v.push_back(i);
    }
    return v;
  }

// identifies last element in an iteration 
template <typename Iter, typename Cont>
bool is_last(Iter iter, const Cont& cont){
  return (iter != cont.end()) && (next(iter) == cont.end());
}

//
// Class implementation
//

cadidaq::settings::settings(std::string name) : name(name)
{
  // Register a constant attribute that identifies our digitizer in the logs
  lg.add_attribute("Digitizer", boost::log::attributes::constant<std::string>(name));
}

void cadidaq::settings::print(){
  // convert all settings to a PTree
  pt::iptree *node = createPTree();
  std::cout << " Config for '" << name << "'" << std::endl;
  /* Loop over all sub sections and keys */
  for (auto& key : *node){
    std::cout << "\t" << key.first << " = " << key.second.get_value<std::string>() << std::endl;
  }
  delete node;
};

void cadidaq::settings::parse(pt::iptree *node){
  processPTree(node, parseDirection::READING);
}

pt::iptree* cadidaq::settings::createPTree(){
  pt::iptree *node = new pt::iptree();
  processPTree(node, parseDirection::WRITING);
  return node;
}

void cadidaq::settings::fillPTree(pt::iptree *node){
  processPTree(node, parseDirection::WRITING);
}

template <class CAEN_ENUM> boost::optional<CAEN_ENUM> cadidaq::settings::iFindStringInBimap(boost::bimap< std::string, CAEN_ENUM > map, std::string str){
  typedef typename boost::bimap< std::string, CAEN_ENUM >::left_const_iterator const_iterator;
  for( const_iterator i = map.left.begin(), iend = map.left.end(); i != iend; ++i ){
      if(boost::iequals(boost::algorithm::to_lower_copy(i->first), str))
        return i->second;
    }
  return boost::none;
}


template <class VALUE> void cadidaq::settings::parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, parseDirection direction, defaultBase base){
  if (direction == parseDirection::READING){
    // get the setting's value from the ptree
    try{
      settingValue = node->get<VALUE>(settingName);
    }
    catch (const pt::ptree_bad_data& e){
      CFG_LOG_ERROR << "Parsing '" << settingName << "' yielded Bad_Data exception (" << e.what() << ") for value: " << node->get<std::string>(settingName);
      return;
    }
    catch (const pt::ptree_bad_path& e){
      CFG_LOG_DEBUG << "Could not find key '" << settingName << "'";
      return;
    }
    if (settingValue) {
      CFG_LOG_DEBUG << "found key " << settingName << " with value '" << *settingValue << "'";
    }
    // erase value from ptree as it has been successfully parsed
    // NOTE: this will only erase a single instance of the key; if the key exists several times, the other entries remain!
    node->erase(settingName);
  } else {
    // direction: WRITING
    // add key to ptree if the setting's value has been set
    if (settingValue) {
      if (base == defaultBase::HEX){
        std::stringstream ss;
        ss << std::hex << std::showbase << settingValue; // might need e.g. std::setfill ('0') and std::setw(sizeof(your_type)*2)
        node->put(settingName, ss.str());
      } else {
        node->put(settingName, *settingValue);
      }
    } else {
      CFG_LOG_WARN << "Value for '" << settingName << "' not defined when generating configuration. Setting will be omitted in output.";
    }
  }
}


template <class VALUE> void cadidaq::settings::parseSetting(std::string settingName, pt::iptree *node, std::vector<boost::optional<VALUE>>& settingValue, parseDirection direction, defaultBase base){
  if (direction == parseDirection::READING){
    // get the setting's value from the ptree by looping over all entries of "settingName[RANGE]"
    // important: to keep the iteration stable even when erasing parsed entries, the iterator increment is handled below
    for (auto key = node->begin(); key != node->end();){
      if (boost::istarts_with(key->first, settingName)){
        CFG_LOG_DEBUG << "Found a matching key: '" << key->first << "' with value '" << key->second.get_value<std::string>() << "'";
        std::string range = boost::erase_head_copy(key->first, settingName.length());
        // remove the brackets or parenthesis and any remaining whitespace
        boost::trim_left_if(range,boost::is_any_of("[("));
        boost::trim_right_if(range,boost::is_any_of(")]"));
        // make sure that there are no characters present that we can't parse
        if (!boost::all(range,boost::is_any_of(",-")||boost::is_digit() || boost::is_space())){
          CFG_LOG_ERROR << "Could not parse range '" << range << "' specified in setting '" << key->first << "' with value '" << key->second.get_value<std::string>() << "'. Only allowed characters are '-', ',' and digits.";
          continue;
        }
        // now split the range into individual channel numbers
        std::vector<int> v = expandRange(range);

        // output the parsed range for debugging purposes
        std::stringstream expandedrange;
        for(auto x:v)
          expandedrange << std::to_string(x) << " ";
        CFG_LOG_DEBUG << "Expanded range '" << range << "' into " << expandedrange.str();

        // loop over parsed range and set the values in the settings vector
        for(auto x:v){
          try{
            settingValue.at(x) = key->second.get_value<VALUE>();
          }
          catch (const std::out_of_range& e) {
            CFG_LOG_ERROR << "Channel number '" << std::to_string(x) << "' in setting '" << settingName << "' is out of range!";
            continue;
          }
          catch (const pt::ptree_bad_data& e) {
            CFG_LOG_ERROR << "Error parsing setting '" << settingName << "': " << e.what() << ". Offending value: " << key->second.get_value<std::string>();
            break;
          }
        }
        // erase the now-parsed key from the ptree node
        // important: to keep the iteration stable over erases we have to use the iterator returned from the erase operation!
        key = node->erase(key);
      } // match settingName*
      else {
        // non-match, go to next key
        ++key;
      }
    } // for (key:node)
  } else {
    // direction: WRITING
    // TODO: write range compression to get setting string as in "settingName[RANGE]"
    // add key to ptree if the setting's value has been set
    for (auto it = settingValue.begin(); it != settingValue.end(); ++it) {
      auto index = std::distance(settingValue.begin(), it);
      if (*it) {
        if (base == defaultBase::HEX){
          std::stringstream ss;
          ss << std::hex << std::showbase << **it; // might need e.g. std::setfill ('0') and std::setw(sizeof(your_type)*2)
          node->put(settingName + "[" + std::to_string(index) + "]", ss.str());
        } else {
          node->put(settingName + "[" + std::to_string(index) + "]", **it);
        }
      } else {
        CFG_LOG_WARN << "Value for '" << settingName << "' not defined for channel #" << std::to_string(index) << " when generating configuration property tree. Setting will be omitted in output.";
      }
    }
  }
}


template <class CAEN_ENUM, typename VALUE> void cadidaq::settings::parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, boost::bimap< std::string, CAEN_ENUM > map, parseDirection direction){
  if (direction == parseDirection::READING){
    boost::optional<std::string> str;
    // get the setting's value from the ptree and append
    parseSetting(settingName, node, str, direction);
    if (str){
      settingValue = iFindStringInBimap(map, std::string("CAEN_DGTZ_") + *str); // match the enum nanming convention in CAEN's driver
      if (settingValue){
        CFG_LOG_DEBUG << "Setting " << settingName << " with value '" << *str << "' converted to value " << *settingValue << " (" << map.right.at(static_cast<CAEN_ENUM>(*settingValue)) << ")";
      } else {
        CFG_LOG_ERROR << "Could not parse value of setting " << settingName << ": '" << *str << "' (unknown value) ";
        // generate a string of known options from the CAEN enum map
        std::stringstream knownOptions;
        typedef typename boost::bimap< std::string, CAEN_ENUM >::left_const_iterator const_iterator;
        for( const_iterator i = map.left.begin(), iend = map.left.end(); i != iend; ++i ){
          knownOptions << boost::erase_head_copy(i->first, std::string("CAEN_DGTZ_").length());
          if (!is_last(i, map.left)) knownOptions << ", ";
        }
        CFG_LOG_ERROR << "-> did you mean either of " << knownOptions.str() << "?";
      }
    } else {
      settingValue = boost::none;
    }
  } else {
    // direction: WRITING

    // check that setting's value (boost::optional) is actually set
    if (!settingValue) return;
    // find the string corresponding to the setting's enum value in the bimap
    boost::optional<std::string> strvalue = map.right.at(static_cast<CAEN_ENUM>(*settingValue));
    // remove the first part originating from CAEN's enum naming convention ("CAEN_DGTZ_")
    strvalue->erase(0,10);
    // add key to ptree
    parseSetting(settingName, node, strvalue, direction);
  }
}


void cadidaq::connectionSettings::verify(){

  if (!linkType){
    CFG_LOG_ERROR << "Missing (or invalid) non-optional setting 'LinkType' in section '" << name << "'";
    throw std::invalid_argument(std::string("Missing (or invalid) setting for 'LinkType'"));
  }
  if (*linkType == CAEN_DGTZ_USB){
    if (conetNode && *conetNode != 0){
      CFG_LOG_DEBUG << "When using LinkType=USB, ConetNode needs to be '0'! Fixed.";
    }
    // set the correct values for the chosen linktype
    conetNode = 0;
  }
  if (!vmeBaseAddress){
    CFG_LOG_DEBUG << "VMEBaseAddress connection option not set, assuming '0'";
    vmeBaseAddress = 0;
  }
  if (!linkNum){
    CFG_LOG_WARN << "LinkNum connection option not set, assuming '0'";
    linkNum = 0;
  }
  CFG_LOG_DEBUG << "Done with verifying connection settings.";
}

void cadidaq::connectionSettings::processPTree(pt::iptree *node, parseDirection direction){
    // this routine implements the calls to ParseSetting for individual settings read from config or stored internally

    cadidaq::CaenEnum2str converter;

    parseSetting("LinkType", node, linkType, converter.bm_CAEN_DGTZ_ConnectionType, direction);
    parseSetting("LinkNum", node, linkNum, direction);
    parseSetting("ConetNode", node, conetNode, direction);
    parseSetting("VMEBaseAddress", node, vmeBaseAddress, direction, defaultBase::HEX);
    CFG_LOG_DEBUG << "Done with processing connection settings ptree";

  }


cadidaq::registerSettings::registerSettings(std::string name, uint nchannels) : cadidaq::settings(name) {
    // CAEN digitizer channel settings
    chEnable.resize(nchannels);
    chPosPolarity.resize(nchannels);
    chNegPolarity.resize(nchannels);
    chDCOffset.resize(nchannels);
    chTriggerThreshold.resize(nchannels);

    // CAEN Standard firmware specific settings
    chZLEThreshold.resize(nchannels);
    chZLEForward.resize(nchannels);
    chZLEBackward.resize(nchannels);
    chZLEPosLogic.resize(nchannels);
    chZLENegLogic.resize(nchannels);
    chBaselineCalcMin.resize(nchannels);
    chBaselineCalcMax.resize(nchannels);
    chPSDTotalStart.resize(nchannels);
    chPSDTotalStop.resize(nchannels);
    chPSDTailStart.resize(nchannels);
    chPSDTailStop.resize(nchannels);

    // CAEN DPP-PSD firwmare specific settings
    chRecordLength.resize(nchannels);
    chBaselineSamples.resize(nchannels);
    chChargeSensitivity.resize(nchannels);
    chPSDCut.resize(nchannels);
    chTriggerConfig.resize(nchannels);
    chTriggerValidation.resize(nchannels);
    chShortGate.resize(nchannels);
    chLongGate.resize(nchannels);
    chPreTrigger.resize(nchannels);
    chGateOffset.resize(nchannels);
}



void cadidaq::registerSettings::processPTree(pt::iptree *node, parseDirection direction){
  // this routine implements the calls to ParseSetting for individual settings read from config or stored internally

  cadidaq::CaenEnum2str converter;
  parseSetting("EnableChannel", node, chEnable, direction);
  parseSetting("SWTriggerMode", node, swTriggerMode, converter.bm_CAEN_DGTZ_TriggerMode_t, direction);
  CFG_LOG_DEBUG << "Done with processing register settings ptree";

}

void cadidaq::registerSettings::verify(){
  // TODO: implement "light" checks on e.g. critical options that are valid for all supported digitizer types/families
  CFG_LOG_DEBUG << "Done with verifying register settings.";
}
