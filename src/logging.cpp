// BOOST logging
#include <boost/log/expressions.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/constant.hpp>
// BOOST time formatting
#include <boost/log/support/date_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <logging.hpp>

// Define the attribute keywords
BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(digitizer, "Digitizer", std::string)

void digitizer_formatter(const logging::record_view& record,
                        logging::formatting_ostream& stream)
{
  auto digi = record[digitizer];
  if (digi)
    stream << "." << record[digitizer];
}

void coloring_formatter(const logging::record_view& record,
                        logging::formatting_ostream& stream)
{
  auto severity = record[logging::trivial::severity];
  assert(severity);

  stream << "\e[1m";

  switch (severity.get())
    {
    case logging::trivial::severity_level::trace:
      stream << "\e[97m";
      break;
    case logging::trivial::severity_level::debug:
      stream << "\e[34m";
      break;
    case logging::trivial::severity_level::info:
      stream << "\e[32m";
      break;
    case logging::trivial::severity_level::warning:
      stream << "\e[93m";
      break;
    case logging::trivial::severity_level::error:
      stream << "\e[91m";
      break;
    case logging::trivial::severity_level::fatal:
      stream << "\e[41m";
      break;
    }
}

void coloring_formatter_terminate(const logging::record_view& record,
                        logging::formatting_ostream& stream)
{
  stream << "\e[0m";
}

void init_console_logging(){
  // init BOOST logging
  boost::log::add_common_attributes();
  // Create a minimal severity table filter
  typedef boost::log::expressions::channel_severity_filter_actor< std::string, boost::log::trivial::severity_level > min_severity_filter;
  min_severity_filter min_severity = boost::log::expressions::channel_severity_filter(channel, severity);

  // Set up the minimum severity levels for different channels
  min_severity["cfg"] = boost::log::trivial::debug;
  min_severity["main"] = boost::log::trivial::info;

  auto consoleLog = boost::log::add_console_log(
                  std::clog,
                  boost::log::keywords::filter = min_severity || severity >= boost::log::trivial::fatal,
                  boost::log::keywords::format =
                  (
                   boost::log::expressions::stream
                   << expr::wrap_formatter(&coloring_formatter)
                   << line_id << " "
                   << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                   << ": <" << severity
                   << "> [" << channel << expr::wrap_formatter(&digitizer_formatter) << "] "
                   << boost::log::expressions::smessage
                   << expr::wrap_formatter(&coloring_formatter_terminate)
                   )
                  );
}

