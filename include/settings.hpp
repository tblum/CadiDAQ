// settings.hpp
#ifndef CADIDAQ_SETTINGS_H
#define CADIDAQ_SETTINGS_H

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

#include <boost/bimap.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

namespace pt = boost::property_tree;

namespace cadidaq {
  class settings;
  class digitizerSettings;
  class connectionSettings;
  class registerSettings;
}

class cadidaq::settings {
public:
  settings(std::string name);
  ~settings(){;}
  void parse(pt::iptree *node);
  pt::iptree* createPTree();
  void fillPTree(pt::iptree *node);
  virtual void verify(){};
  void print();
  std::string getName(){return name;}
protected:
  std::string name;
  boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;
  enum class parseDirection {READING, WRITING};
  enum class defaultBase {NONE, HEX};
  template <class VALUE> void parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, parseDirection direction, defaultBase = defaultBase::HEX);
  template <class VALUE> void parseSetting(std::string settingName, pt::iptree *node, std::vector<boost::optional<VALUE>>& settingValue, parseDirection direction, defaultBase = defaultBase::HEX);
  template <class CAEN_ENUM> boost::optional<CAEN_ENUM> iFindStringInBimap(boost::bimap< std::string, CAEN_ENUM > map, std::string str);
  template <class CAEN_ENUM, typename VALUE> void parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, boost::bimap< std::string, CAEN_ENUM > map, parseDirection direction);
private:
  virtual void processPTree(pt::iptree *node, parseDirection direction){};
};

class cadidaq::connectionSettings : public settings {
public:
  connectionSettings(std::string name) : cadidaq::settings(name) {}
  ~connectionSettings(){;}

  void verify();

  boost::optional<int>      linkType;
  boost::optional<int>      linkNum;
  boost::optional<int>      conetNode;
  boost::optional<uint32_t> vmeBaseAddress;
private:
  virtual void processPTree(pt::iptree *node, parseDirection direction);
};

class cadidaq::registerSettings : public settings {
public:
  registerSettings(std::string name, uint nchannels);
  ~registerSettings(){;}

  void verify();

  // CAEN channel settings
  std::vector<boost::optional<bool>>  chEnable;
  std::vector<boost::optional<bool>>  chPosPolarity;
  std::vector<boost::optional<bool>>  chNegPolarity;
  std::vector<boost::optional<int>>   chDCOffset;
  std::vector<boost::optional<int>>   chTriggerThreshold;

  // CAEN Standard firmware specific settings
  std::vector<boost::optional<int>>   chZLEThreshold;
  std::vector<boost::optional<int>>   chZLEForward;
  std::vector<boost::optional<int>>   chZLEBackward;
  std::vector<boost::optional<bool>>  chZLEPosLogic;
  std::vector<boost::optional<bool>>  chZLENegLogic;
  std::vector<boost::optional<int>>   chBaselineCalcMin;
  std::vector<boost::optional<int>>   chBaselineCalcMax;
  std::vector<boost::optional<int>>   chPSDTotalStart;
  std::vector<boost::optional<int>>   chPSDTotalStop;
  std::vector<boost::optional<int>>   chPSDTailStart;
  std::vector<boost::optional<int>>   chPSDTailStop;

  // CAEN DPP-PSD firmware specific settings
  std::vector<boost::optional<int>>  chRecordLength;
  std::vector<boost::optional<int>>  chBaselineSamples;
  std::vector<boost::optional<int>>  chChargeSensitivity;
  std::vector<boost::optional<int>>  chPSDCut;
  std::vector<boost::optional<int>>  chTriggerConfig;
  std::vector<boost::optional<int>>  chTriggerValidation;
  std::vector<boost::optional<int>>  chShortGate;
  std::vector<boost::optional<int>>  chLongGate;
  std::vector<boost::optional<int>>  chPreTrigger;
  std::vector<boost::optional<int>>  chGateOffset;

private:
  virtual void processPTree(pt::iptree *node, parseDirection direction);
};

#endif
