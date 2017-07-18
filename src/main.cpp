/**
 * Parse the UTF8 ".ini" file via the Boost property_tree
 */

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>   // std::hex
#include <stdexcept> // exceptions

#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

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

    std::vector<cadidaq::connectionSettings*> vecSettings;
    // get the connection details for each digitizer section
    for (auto& section : iniPTree){
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      pt::iptree &node = iniPTree.get_child(section.first);
      cadidaq::connectionSettings* settings = new cadidaq::connectionSettings(section.first);
      settings->parse(&node);
      settings->verify();
      vecSettings.push_back(settings);
    }

    // write the config back to another file
    pt::iptree ptwrite; // create a new tree
    BOOST_FOREACH(cadidaq::connectionSettings *settings, vecSettings){
      pt::iptree *node = settings->createPTree();
      ptwrite.put_child(settings->getName(), *node);
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

