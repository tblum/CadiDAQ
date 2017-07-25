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

#include <helper.hpp>       // CadiDAQ helper functions

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


enum class comDirection {READING, WRITING};

template <typename T> void programWrapper(caen::Digitizer* instance, void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), T &value, comDirection direction){
  try{
    if (direction == comDirection::WRITING){
      // WRITING
      (instance->*write)(value);
    } else {
      // READING
      T test = (instance->*read)();
    }
  }
  catch (caen::Error& e){
    // TODO: more fine-grained error handling, more info on log
    MAIN_LOG_ERROR << "Caught exception when communicating with digitizer " << instance->modelName() << ", serial " << instance->serialNumber() << ": " << e.what();
  }
}

template <typename T> void programWrapper(caen::Digitizer* instance, void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), boost::optional<T> &value, comDirection direction){
  // simple additional check to program only if:
  // - optional value is set
  // - or the value is being read out from the digitizer
  if (value){
    programWrapper(instance, write, read, *value, direction);
  } else if (direction == comDirection::READING){
    // reading works even without the value being set (yet)
    programWrapper(instance, write, read, *value, direction);
  }
}

void programMaskWrapper(caen::Digitizer* digitizer, void (caen::Digitizer::*write)(uint32_t), uint32_t (caen::Digitizer::*read)(), std::vector<boost::optional<bool>> &vec, comDirection direction){
  uint32_t mask = 0;
  // derive the mask in case we are writing it
  if (direction == comDirection::WRITING)
    mask = vec2Mask(vec);
  programWrapper(digitizer, &caen::Digitizer::setChannelEnableMask, &caen::Digitizer::getChannelEnableMask, mask, direction);
  // if reading: now store the retrieved mask it in the vector
  if (direction == comDirection::READING)
    mask2Vec(mask, vec);
}


void programSettings(caen::Digitizer* digitizer, cadidaq::registerSettings* settings, comDirection direction){

  programWrapper(digitizer, &caen::Digitizer::setSWTriggerMode, &caen::Digitizer::getSWTriggerMode, settings->swTriggerMode, direction);

  if (digitizer->groups() == 1){
    // no grouped channels

    programMaskWrapper(digitizer, &caen::Digitizer::setChannelEnableMask, &caen::Digitizer::getChannelEnableMask, settings->chEnable, direction);
  } else {
    // channels are grouped
    // TODO: verify that channel vector -> group mask conversion is consistent and the same as channel -> channel mask, else warn about misconfiguration
  }
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
      caen::Digitizer* digitizer = nullptr;
      MAIN_LOG_INFO << "Establishing connection to digitizer '" << section.first
                    << "' (linkType=" << *linksettings->linkType
                    << ", linkNum=" << *linksettings->linkNum
                    << ", ConetNode=" << *linksettings->conetNode
                    << ", VMEBaseAddress=" << std::to_string(*linksettings->vmeBaseAddress) << ")";
      try{
        digitizer = caen::Digitizer::open(*linksettings->linkType, *linksettings->linkNum, *linksettings->conetNode, *linksettings->vmeBaseAddress);
      }
      catch (caen::Error& e){
        MAIN_LOG_ERROR << "Caught exception when establishing communication with digitizer " << linksettings->getName() << ": " << e.what();
        if (!digitizer){
          // TODO: more fine-grained error handling, more info on log
          // won't be able to handle this much more gracefully than:
          exit(EXIT_FAILURE);
        }
      }
      uint nchannels = digitizer->channels();
      cadidaq::registerSettings* regsettings = new cadidaq::registerSettings(section.first, nchannels);
      regsettings->parse(&node);
      regsettings->verify();
      programSettings(digitizer, regsettings, comDirection::WRITING);
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

