// settings.hpp
#ifndef CADIDAQ_DIGITIZER_H
#define CADIDAQ_DIGITIZER_H

#include <string>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

#include <settings.hpp>

namespace caen {
  class Digitizer;
}

namespace cadidaq {

  class digitizer {
  public:
    digitizer(std::string name);
    ~digitizer();
    void             configure(pt::iptree *node);
    pt::iptree*      retrieveConfig();
    caen::Digitizer* getDevice(){return dg;}
    std::string      getName(){return name;}
  private:
    enum class comDirection {READING, WRITING};

    void verifySettings();

    template <typename T>
    void programWrapper(void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), T &value, comDirection direction);

    template <typename T, typename C>
    void programWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), C channel, T &value, comDirection direction);

    template <typename T>
    void programWrapper(void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), boost::optional<T> &value, comDirection direction);

    void programMaskWrapper(void (caen::Digitizer::*write)(uint32_t), uint32_t (caen::Digitizer::*read)(), cadidaq::settingsBase::optionVector<bool> &vec, comDirection direction);

    template <typename T, typename C>
    void programLoopWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), cadidaq::settingsBase::optionVector<T> &vec, comDirection direction, bool ignoreGroups);

    void programSettings(comDirection direction);

    caen::Digitizer*    dg;
    connectionSettings* lnk;
    registerSettings*   reg;
    std::string         name;
    boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;
  };
}

#endif
