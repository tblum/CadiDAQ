/**
 * Parse the UTF8 ".ini" file via the Boost property_tree
 */

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>   // std::hex
#include <stdexcept> // exceptions

#include <bitset>
#include <iterator>  // distance

#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <boolTranslator.hpp> // changes 'bool' interpretation in ptrees

#include "caen.hpp"

#include <logging.hpp>
#include <settings.hpp>

namespace po = boost::program_options;
namespace pt = boost::property_tree;

#define MAIN_LOG_DEBUG                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "main", boost::log::trivial::debug)
#define MAIN_LOG_INFO                                           \
  BOOST_LOG_CHANNEL_SEV(lg, "main", boost::log::trivial::info)
#define MAIN_LOG_WARN                                             \
  BOOST_LOG_CHANNEL_SEV(lg, "main", boost::log::trivial::warning)
#define MAIN_LOG_ERROR                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "main", boost::log::trivial::error)
#define MAIN_LOG_FATAL                                          \
  BOOST_LOG_CHANNEL_SEV(lg, "main", boost::log::trivial::fatal)


//
// programming configuration into digitizer
//

/* converts a vector of optional<bool> into a uint32 bit mask.
 defaults to 0 if optional not set */
  uint32_t vec2Mask(std::vector<boost::optional<bool>> vec, uint groupsize = 1){
    std::bitset<32> mask(0);
    for (auto it = vec.begin(); it != vec.end(); ++it) {
      auto index = std::distance(vec.begin(), it);
      // set value if set and 'true' (else it stays 0)
      if (*it && **it) {
        uint groupidx = index%groupsize;
        // treat "special case" of all channels belonging to the same group (e.g. no groups, just channels)
        if (groupsize == 1)
          groupidx = index;
        // create a bit pattern for the group the channel belongs to
        uint32_t shift = (((uint32_t)1 << groupsize) - 1) << groupidx;
        // set that pattern
        mask |= shift;
      }
    }
    return mask.to_ulong();
  }

template <typename T> void programWrapper(caen::Digitizer* instance, void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), T value){
  (instance->*write)(value);
  T test = (instance->*read)();
  std::cout << std::to_string(test) << std::endl;
}


void programSettings(caen::Digitizer* digitizer, cadidaq::registerSettings* settings){
  program(digitizer, &caen::Digitizer::writeTestMask, &caen::Digitizer::readTestMask, vec2Mask(settings->chEnable));
}

//
// reading config file
//

void read_ini_file(const char *filename)
{

    /* Open the UTF8 .ini file */
    std::ifstream iniStream(filename);

    /* Parse the .ini file via boost::property_tree::ini_parser */
    pt::iptree iniPTree; // ptree w/ case-insensitive comparisons
    pt::ini_parser::read_ini(iniStream, iniPTree);

    printf("\n\nFull config:\n");
    /* Loop over all sections and keys */
    for (auto& section : iniPTree){
      std::cout << '[' << section.first << "]" << std::endl;
      for (auto& key : section.second)
        std::cout << key.first << "=" << key.second.get_value<std::string>() << std::endl;
    }

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

    std::vector<cadidaq::connectionSettings*> vecLnkSettings;
    std::vector<cadidaq::registerSettings*> vecRegSettings;
    // get the connection details for each digitizer section
    for (auto& section : iniPTree){
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      pt::iptree &node = iniPTree.get_child(section.first);
      cadidaq::connectionSettings* linksettings = new cadidaq::connectionSettings(section.first);
      linksettings->parse(&node);
      linksettings->verify();
      // TODO: actually establish connection (not using dummy as of now), determine number of available channels
      caen::Digitizer* digitizer = caen::Digitizer::USB(0);
      uint nchannels = digitizer->channels();
      cadidaq::registerSettings* regsettings = new cadidaq::registerSettings(section.first, nchannels);
      regsettings->parse(&node);
      regsettings->verify();
      programSettings(digitizer, regsettings);
      /* Loop over all sub sections and keys that remained after parsing */
      for (auto& key : node){
        MAIN_LOG_WARN << "Unknown setting in section " << section.first << " ignored: \t" << key.first << " = " << key.second.get_value<std::string>();
      }
      vecLnkSettings.push_back(linksettings);
      vecRegSettings.push_back(regsettings);
    }

    // write the config back to another file
    pt::iptree ptwrite; // create a new tree
    BOOST_FOREACH(cadidaq::connectionSettings *settings, vecLnkSettings){
      pt::iptree *node = settings->createPTree();
      ptwrite.put_child(settings->getName(), *node);
    }
    BOOST_FOREACH(cadidaq::registerSettings *settings, vecRegSettings){
      pt::iptree &node = ptwrite.get_child(settings->getName());
      settings->fillPTree(&node);
    }
    pt::ini_parser::write_ini("output.ini", ptwrite);
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

    init_console_logging();

    std::string iniFile = vm["file"].as<std::string>().c_str();
    std::cout << "Read ini file: " << iniFile << std::endl;
    read_ini_file(iniFile.c_str());
    MAIN_LOG_INFO << "Program loop terminated. Have a nice day :)";
    return 0;
}

