#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

/* namespace alias (commonly used in boost examples) */
namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

static boost::log::sources::severity_channel_logger< boost::log::trivial::severity_level, std::string > lg;

void init_console_logging();
