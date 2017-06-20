/**
 * Parse the UTF8 ".ini" file via the Boost property_tree
 */

#include <string>
#include <fstream>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>

/* namespace alias */
namespace po = boost::program_options;
namespace pt = boost::property_tree;

class ConnectionSettings {
public:
  ConnectionSettings(){;}
  ~ConnectionSettings(){;}

  uint8_t LinkType;
  uint8_t LinkNum;
  uint8_t ConetNode;
  uint32_t VMEBaseAddress;
};

void read_ini_file(const char *filename)
{

    /* Open the UTF8 .ini file */
    std::ifstream iniStream(filename);

    /* Parse the .ini file via boost::property_tree::ini_parser */
    pt::ptree iniPTree;
    pt::ini_parser::read_ini(iniStream, iniPTree);

    /* Read the string */
    boost::optional<std::string> strVal = 
      iniPTree.get_optional<std::string>("app.string");
    std::string ret = strVal.get_value_or(std::string("defaultStrVal"));

    printf("app.string=%s\n", ret.c_str());

    /* Read the number */
    boost::optional<uint32_t> numVal = 
        iniPTree.get_optional<uint32_t>("app.number");
    const uint32_t numValDefault = 100;
    std::cout << "app.number="
        << numVal.get_value_or(numValDefault)
        << std::endl;
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

