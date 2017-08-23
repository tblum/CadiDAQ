// settings.hpp
#ifndef CADIDAQ_DIGITIZER_H
#define CADIDAQ_DIGITIZER_H

#include <string>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

#include <settings.hpp>
#include <helper.hpp>       // helper functions
#include <caen.hpp>

#define DG_LOG_DEBUG BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::debug)
#define DG_LOG_INFO  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::info)
#define DG_LOG_WARN  BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::warning)
#define DG_LOG_ERROR BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::error)
#define DG_LOG_FATAL BOOST_LOG_CHANNEL_SEV(lg, "dig", boost::log::trivial::fatal)

namespace cadidaq {

    class digitizer {
    public:
        digitizer(std::string name);
        ~digitizer();
        void             configure(pt::iptree *node);
        pt::iptree*      retrieveConfig();
        caen::Digitizer* getDevice(){return dg;}
        std::string      getName(){return name;}
        enum class comDirection {READING, WRITING};
    private:
        void verifySettings();

        template <typename T>
        void programWrapper(void (caen::Digitizer::*write)(T), T (caen::Digitizer::*read)(), boost::optional<T> &value, comDirection direction){
            try{
                if (direction == comDirection::WRITING){
                    // WRITING
                    if (value)
                        (dg->*write)(*value);
                } else {
                    // READING
                    value = (dg->*read)();
                }
            }
            catch (caen::Error& e){
                // TODO: more fine-grained error handling, more info on log
                DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
                if (direction == comDirection::WRITING)
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " with argument '" << std::to_string(*value) << "' caused exception: " << e.what();
                else
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " caused exception: " << e.what();
                // setting assumed to be invalid regardless whether we read or write it:
                value = boost::none;
            }
        }
        
        template <typename T, typename C>
        void programWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), C channel, boost::optional<T> &value, comDirection direction){
            try{
                if (direction == comDirection::WRITING){
                    // WRITING
                    if (value)
                        (dg->*write)(channel, *value);
                } else {
                    // READING
                    value = (dg->*read)(channel);
                }
            }
            catch (caen::Error& e){
                // TODO: more fine-grained error handling, more info on log
                DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
                if (direction == comDirection::WRITING)
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " for channel/group " << channel << " with argument '" << std::to_string(*value) << "' caused exception: " << e.what();
                else
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " for channel/group " << channel << " caused exception: " << e.what();
                // setting assumed to be invalid regardless whether we read or write it:
                value = boost::none;
            }
        }
        
        template <typename T1, typename T2>
        void programWrapper(void (caen::Digitizer::*write)(T1, T2), void (caen::Digitizer::*read)(T1&, T2&), boost::optional<T1> &value1, boost::optional<T2> &value2, comDirection direction){
            try{
                if (direction == comDirection::WRITING){
                    // WRITING
                    if (value1 && value2)
                        (dg->*write)(*value1, *value2);
                } else {
                    // READING
                    T1 v1;
                    T2 v2;
                    (dg->*read)(v1, v2);
                    value1 = v1;
                    value2 = v2;
                }
            }
            catch (caen::Error& e){
                // TODO: more fine-grained error handling, more info on log
                DG_LOG_ERROR << "Caught exception when communicating with digitizer " << dg->modelName() << ", serial " << dg->serialNumber() << ":";
                if (direction == comDirection::WRITING)
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " with arguments '" << std::to_string(*value1) << "', '" << std::to_string(*value2) << "' caused exception: " << e.what();
                else
                    DG_LOG_ERROR << "\t Calling " << "e.where()" << " caused exception: " << e.what();
                // setting assumed to be invalid regardless whether we read or write it:
                value1 = boost::none;
                value2 = boost::none;
            }
        }
        void programMaskWrapper(void (caen::Digitizer::*write)(uint32_t), uint32_t (caen::Digitizer::*read)(), cadidaq::settingsBase::optionVector<bool> &vec, comDirection direction);
        
        template <typename T, typename C>
        void programLoopWrapper(void (caen::Digitizer::*write)(C, T), T (caen::Digitizer::*read)(C), cadidaq::settingsBase::optionVector<T> &vec, comDirection direction, bool ignoreGroups = false){
            int ngroups = dg->groups();
            // if groups are to be ignored
            if (ignoreGroups)
                ngroups = 1;
            // verify that the vector can be put into group structure of the device (if channels are grouped)
            if (ngroups>1){
                for (int i = 0; i<ngroups; i++){
                    if (!allValuesSame(vec.first,i*dg->channelsPerGroup(), (i+1)*dg->channelsPerGroup()))
                        DG_LOG_WARN << "The channels in the range " << i*dg->channelsPerGroup() << " and " << (i+1)*dg->channelsPerGroup() << " for '" << vec.second << "' are set to different values -> cannot consistently convert to groups supported by the device!";
                }
            }
            // loop over vector's entries and READ/WRITE values from/to digitzer
            for (auto it = vec.first.begin(); it != vec.first.end(); ++it) {
                auto channel = std::distance(vec.first.begin(), it);
                if (direction == comDirection::WRITING){
                    if (!*it)
                        continue; // skip and leave default
                }
                // map channel number to group index (or set to one if NGroups==1)
                C group = channel/ngroups;
                if (direction == comDirection::READING && channel%ngroups != 0)
                    continue; // only read once per group
                
                // perform the call to the digitizer
                programWrapper(write, read, group, *it, direction);
                if (direction == comDirection::READING){
                    if (ngroups > 1){
                        // set the other values in the group
                        for (int i = group*dg->channelsPerGroup(); i<(group+1)*dg->channelsPerGroup(); i++){
                            vec.first.at(i) = *it;
                        }
                    }
                }
            }
        }
        
        void programSettings(comDirection direction);
        
        caen::Digitizer*    dg;
        connectionSettings* lnk;
        registerSettings*   reg;
        std::string         name;
        boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;
    };
}

#endif
