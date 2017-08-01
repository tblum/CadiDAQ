/**
 * Parse the UTF8 ".ini" file via the Boost property_tree
 */

#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept> // exceptions

#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <logging.hpp>
#include <settings.hpp>
#include <digitizer.hpp>

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
// reading config file
//

void read_ini_file(const char *filename)
{

    /* Open the UTF8 .ini file */
    std::ifstream iniStream(filename);

    /* Parse the .ini file via boost::property_tree::ini_parser */
    pt::iptree iniPTree; // ptree w/ case-insensitive comparisons
    pt::ini_parser::read_ini(iniStream, iniPTree);

    // parse the config file to determine number of digitizers
    int NDigitizer = 0;
    for (auto& section : iniPTree){
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      NDigitizer++;
    }
    MAIN_LOG_INFO << "Configuration for " << NDigitizer << " digitizer(s) found in config file.";

    std::vector<cadidaq::digitizer*> vecDigi;
    // get the connection details for each digitizer section
    for (auto& section : iniPTree){
      // ignoring "daq" settings for main application
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("daq")))
        continue;
      // ignoring "general" section for common digitizer settings (for now)
      if(boost::iequals(boost::algorithm::to_lower_copy(section.first), std::string("general")))
        continue;
      // retrieve this section's settings
      std::string digName = section.first;
      pt::iptree &nodeDigi = iniPTree.get_child(digName);
      MAIN_LOG_INFO << "Found '" << digName << "' section in config file.";
      // join the section with any setting in the general section (overwriting the latter where appropriate)
      pt::iptree *node = new pt::iptree(); // need new pttree as not to modify the one read from the ini file
      try {
        // retrieve the "general" section of the config file to initialize defaults
        pt::iptree &nodeGeneral = iniPTree.get_child("GENERAL");
        MAIN_LOG_INFO << "Found 'General' section in config file and applying its values as default.";
        // update the fresh node with settings from the general section
        BOOST_FOREACH( auto& leaf, nodeGeneral ){
            node->put_child( leaf.first, leaf.second );
        }
        // update the node with settings from the digitizer section, possibly overwriting values
        BOOST_FOREACH( auto& leaf, nodeDigi ){
          node->put_child( leaf.first, leaf.second );
        }
      }
      catch (const pt::ptree_bad_path& e){
        MAIN_LOG_DEBUG << "No 'General' section (with options valid for all digitizers) could be found in config file.";
        // just use what is in the digitizer section
        node = &nodeDigi;
      }

      // parse, establish connection and configure digitizer
      cadidaq::digitizer* digi = new cadidaq::digitizer(digName);
      digi->configure(node);
      vecDigi.push_back(digi);

    }

    // TODO: init and run the actual "DAQ" part of the application here

    // write the config back to another file
    std::string outIniFileName = "output.ini";
    MAIN_LOG_INFO << "Reading back configuration from digitizer and writing to output file: " << outIniFileName;
    pt::iptree ptwrite; // create a new tree
    BOOST_FOREACH(cadidaq::digitizer *digi, vecDigi){
      pt::iptree *node = digi->retrieveConfig();
      ptwrite.put_child(digi->getName(), *node);
    }
    pt::ini_parser::write_ini(outIniFileName, ptwrite);
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

