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
}

class cadidaq::settings {
public:
  settings(std::string name);
  ~settings(){;}
  void parse(pt::iptree *node);
  pt::iptree* createPTree();
  virtual void verify(){};
  void print();
  std::string getName(){return name;}
protected:
  std::string name;
  boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;
  enum class parseDirection {READING, WRITING};
  template <class CAEN_ENUM> boost::optional<CAEN_ENUM> iFindStringInBimap(boost::bimap< std::string, CAEN_ENUM > map, std::string str);
  template <class VALUE> void parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, parseDirection direction);
  template <class VALUE> void parseSetting(std::string settingName, pt::iptree *node, boost::optional<VALUE>& settingValue, parseDirection direction, int base);
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
