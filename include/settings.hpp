// settings.hpp
#ifndef CADIDAQ_SETTINGS_H
#define CADIDAQ_SETTINGS_H

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

#include <boost/bimap.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

// CAEN
#include <CAENDigitizerType.h>
#include <caen.hpp>

namespace pt = boost::property_tree;

namespace cadidaq {
  class settingsBase;
  class connectionSettings;
  class registerSettings;
}

/** /class settingsBase
   Base class to hold settings values and provide methods to parse (and output again) boost's property trees for values and verify them for consistency.
 */
class cadidaq::settingsBase {
public:
  settingsBase(std::string name);
  ~settingsBase(){;}
  void parse(pt::iptree *node);
  pt::iptree* createPTree();
  void fillPTree(pt::iptree *node);
  virtual void verify(){};
  void print();
  std::string getName(){return name;}

  /// Define convenience type aliases
  template<class T>
  using option = std::pair< boost::optional<T>, std::string >;

  template<class T>
  using Vec = std::vector< boost::optional<T> >;

  template<class T>
  using optionVector = std::pair< Vec<T>, std::string >;

protected:
  std::string name;
  boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;
  enum class parseDirection {READING, WRITING};
  enum class parseFormat {DEFAULT, HEX, CAENEnum};
  template <class VALUE> void parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, parseDirection direction, parseFormat format = parseFormat::DEFAULT);
  template <typename VALUE> void parseSetting(std::string settingName, pt::iptree *node, std::vector<boost::optional<VALUE>>& settingValue, parseDirection direction, parseFormat format = parseFormat::DEFAULT);
  /// overloaded methods using combined settings/setting's name nomenclature
  template <class VALUE> void parseSetting(option<VALUE>& setting, pt::iptree *node, parseDirection direction, parseFormat format = parseFormat::DEFAULT);
  template <typename VALUE> void parseSetting(optionVector<VALUE>& setting, pt::iptree *node, parseDirection direction, parseFormat format = parseFormat::DEFAULT);
  /// method to parse arbitrary register address-value pairs
  void parseRegisters(pt::iptree *node, std::vector< std::pair< uint32_t, uint32_t >>& registers, parseDirection direction);

private:
  virtual void processPTree(pt::iptree *node, parseDirection direction){};
};


/** /class connectionSettings
    Class to hold settings specifically needed for establishing a link to a digitizer
*/
class cadidaq::connectionSettings : public settingsBase {
public:
  connectionSettings(std::string name) : cadidaq::settingsBase(name) {}
  ~connectionSettings(){;}

  void verify();

  // TODO: change over to 'Option' types
  boost::optional<CAEN_DGTZ_ConnectionType>      linkType;
  boost::optional<int>      linkNum;
  boost::optional<int>      conetNode;
  boost::optional<uint32_t> vmeBaseAddress;
private:
  virtual void processPTree(pt::iptree *node, parseDirection direction);
};

/** /class registerSettings
    Class to hold all configuration settings that can be written to the digitizer device after the connection is established.
*/
class cadidaq::registerSettings : public settingsBase {
public:
  registerSettings(std::string name, uint nchannels);
  ~registerSettings(){;}

  void verify();

  /// arbitrary register address-value pairs
  std::vector<std::pair<uint32_t, uint32_t>> registerValues;

  /// data readout settings
  option<uint32_t>                          maxNumEventsBLT;

  /// trigger settings
  option<CAEN_DGTZ_TriggerMode_t>           swTriggerMode;
  option<CAEN_DGTZ_TriggerMode_t>           externalTriggerMode;
  option<CAEN_DGTZ_IOLevel_t>               ioLevel;
  optionVector<CAEN_DGTZ_TriggerMode_t>     chSelfTrigger;
  optionVector<uint32_t>                    chTriggerThreshold;
  optionVector<CAEN_DGTZ_TriggerPolarity_t> chTriggerPolarity;
  option<CAEN_DGTZ_RunSyncMode_t>           runSyncMode;
  option<CAEN_DGTZ_OutputSignalMode_t>      outSignalMode;

  /// acquisition settings
  option<CAEN_DGTZ_AcqMode_t>               acquisitionMode;
  option<uint32_t>                          recordLength;
  option<uint32_t>                          postTriggerSize;
  optionVector<bool>                        chEnable;
  optionVector<uint32_t>                    chDCOffset;
  option<CAEN_DGTZ_EnaDis_t>                desMode;

  /// DPP-FW settings
  optionVector<uint32_t>                    dppPreTriggerSize;
  optionVector<CAEN_DGTZ_PulsePolarity_t>   dppChPulsePolarity;
  option<caen::DPPAcquisitionMode>           dppAcqMode;
  option<CAEN_DGTZ_DPP_TriggerMode_t>       dppTriggermode;

private:
  virtual void processPTree(pt::iptree *node, parseDirection direction);
};

#endif
