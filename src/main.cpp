/**
 * Parse the UTF8 ".ini" file via the Boost property_tree
 */

#include <string>
#include <fstream>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>

#include <boost/algorithm/string.hpp>

#include <stdexcept>

#include <CaenEnum2Str.hpp> // generated by CMake

/* namespace alias */
namespace po = boost::program_options;
namespace pt = boost::property_tree;

namespace cadidaq {
  class ConnectionSettings;
}

class cadidaq::ConnectionSettings {
public:
  ConnectionSettings(){;}
  ~ConnectionSettings(){;}

  uint8_t LinkType;
  uint8_t LinkNum;
  uint8_t ConetNode;
  uint32_t VMEBaseAddress;
};


template <class CAEN_ENUM> CAEN_ENUM iFindStringInBimap(boost::bimap< std::string, CAEN_ENUM > map, std::string str){
  typedef typename boost::bimap< std::string, CAEN_ENUM >::left_const_iterator const_iterator;
  for( const_iterator i = map.left.begin(), iend = map.left.end(); i != iend; ++i ){
      if(boost::iequals(boost::algorithm::to_lower_copy(i->first), str))
        return i->second;
    }
  // no value found: throw exception
  throw std::out_of_range (std::string("No setting in CAEN driver found to match ") + str);
}

enum class parseDirection {READING, WRITING};

template <class CAEN_ENUM> void ParseSetting(std::string settingName, pt::iptree *node, int& settingValue, boost::bimap< std::string, CAEN_ENUM > map, parseDirection direction){

  if (direction == parseDirection::READING){
    // match the enum nanming convention in CAEN's driver
    std::string str = "CAEN_DGTZ_";
    // get the setting's value from the ptree and append
    str += node->get<std::string>(settingName);
    settingValue = iFindStringInBimap(map, str);
    std::cout << " value of key " << settingName << " with value '" << node->get<std::string>(settingName) << "' converted to value " << settingValue << std::endl;
    // erase value from ptree as it has been successfully parsed
    // NOTE: this will only erase a single instance of the key; if the key exists several times, the other entries remain!
    node->erase(settingName);
  } else {
    // find the string corresponding to the setting's enum value in the bimap
    std::string strvalue = map.right.at(static_cast<CAEN_ENUM>(settingValue));
    // remove the first part originating from CAEN's enum naming convention ("CAEN_DGTZ_")
    strvalue.erase(0,10);
    // add key to ptree
    node->put(settingName, strvalue);
    std::cout << " value of key " << settingName << " with settings value '" << settingValue << "' has been stored in tree as '" << strvalue << "'" << std::endl;
  }
}

  void ParseConnectionSettings(cadidaq::ConnectionSettings* settings, pt::iptree *node, std::string secname, parseDirection direction){
    std::cout << " details for digitizer section: " << secname << std::endl;
    
    /* Loop over all sub sections and keys */
    for (auto& key : *node){
      std::cout << "\t" << key.first << " = " << key.second.get_value<std::string>() << std::endl;
    }

    cadidaq::CaenEnum2str converter;
    int linktype = -1;
    ParseSetting("LinkType", node, linktype, converter.bm_CAEN_DGTZ_ConnectionType, parseDirection::READING);
    
    std::cout << std::endl << " after parsing of first setting: " << std::endl;
    /* Loop over all sub sections and keys */
    for (auto& key : *node){
      std::cout << "\t" << key.first << " = " << key.second.get_value<std::string>() << std::endl;
    }
    
    ParseSetting("LinkType", node, linktype, converter.bm_CAEN_DGTZ_ConnectionType, parseDirection::WRITING);
    std::cout << std::endl << " after adding first setting again: " << std::endl;
    /* Loop over all sub sections and keys */
    for (auto& key : *node){
      std::cout << "\t" << key.first << " = " << key.second.get_value<std::string>() << std::endl;
    }
    
  }

void read_ini_file(const char *filename)
{

    /* Open the UTF8 .ini file */
    std::ifstream iniStream(filename);

    /* Parse the .ini file via boost::property_tree::ini_parser */
    pt::iptree iniPTree; // ptree w/ case-insensitive comparisons
    pt::ini_parser::read_ini(iniStream, iniPTree);

    printf("\n\nFull config::\n");
    /* Loop over all sections and keys */
    for (auto& section : iniPTree){
      std::cout << '[' << section.first << "]" << std::endl;
      for (auto& key : section.second)
        std::cout << key.first << "=" << key.second.get_value<std::string>() << std::endl;
    }

    printf("\n\nReading individual keys:\n");

    /* Read the strings */
    boost::optional<std::string> strVal =
      iniPTree.get_optional<std::string>("digi1.linktype");
    std::string ret = strVal.get_value_or(std::string("default"));

    printf("digi1.LinkType=%s\n", ret.c_str());

    strVal = iniPTree.get_optional<std::string>("digi1.name");
    ret = strVal.get_value_or(std::string("default"));
    printf("digi1.name=%s\n", ret.c_str());

    /* Read the number */
    boost::optional<uint32_t> numVal =
        iniPTree.get_optional<uint32_t>("app.number");
    const uint32_t numValDefault = 111;
    std::cout << "app.number="
        << numVal.get_value_or(numValDefault)
        << std::endl;

    // parse the config file to determine number of digitizers
    int NDigitizer = 0;
    for (auto& section : iniPTree){
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      NDigitizer++;
    }
    std::cout << "Configuration for " << NDigitizer << " digitizer(s) found in config file." << std::endl;

    // get the connection details for each digitizer section
    for (auto& section : iniPTree){
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      pt::iptree &node = iniPTree.get_child(section.first);
      cadidaq::ConnectionSettings* settings = new cadidaq::ConnectionSettings();
      ParseConnectionSettings(settings, &node, section.first, parseDirection::READING);
    }



}

int main(int argc, char **argv)
{
    po::options_description desc("MainOptions");
    desc.add_options()
        ("help,h", "Print help message")
        ("file,f", 
            po::value<std::string>()->default_value("test.ini"),
            "The test .ini file");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch (po::error &e)
    {
        /* Invalid options */
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cout << "Boost property_tree tester:" << std::endl
            << desc << std::endl;
        return 0;
    }

    if (vm.count("help"))
    {
        /* print usage */
        std::cout << "Boost property_tree tester:" << std::endl
                  << desc << std::endl;
        return 0;
    }

    std::string iniFile = vm["file"].as<std::string>().c_str();
    std::cout << "Read ini file: " << iniFile << std::endl;
    read_ini_file(iniFile.c_str());
    return 0;
}

