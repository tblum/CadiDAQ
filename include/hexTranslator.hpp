// provides a translator to read hex/int values from strings in Boost property trees
// code adopted from https://stackoverflow.com/a/9754025

#ifndef CADIDAQ_hexTranslator_hpp
#define CADIDAQ_hexTranslator_hpp

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> // boost::starts_with
#include <boost/lexical_cast.hpp>


// Custom translator for hex (only supports std::string)
struct hexTranslator
{
    typedef std::string internal_type;
    typedef int         external_type;

    // Converts a (hex)string to int
    boost::optional<external_type> get_value(const internal_type& str)
    {
        if (!str.empty())
        {
          external_type i;
          if (boost::istarts_with(str, "0x")){
            // treat as hex
            std::size_t* pos;
            try{
              i = std::stoi(str, pos, 16);
              }
            catch (std::invalid_argument& e){
              // no conversion performed
              return boost::optional<external_type>(boost::none);
            }
            catch(std::out_of_range& e){
              // if the converted value would fall out of the range of the result type
              // or if the underlying function (std::strtol or std::strtoull) sets errno
              // to ERANGE.
              return boost::optional<external_type>(boost::none);
            }
            if (*pos != str.length()){
              // not all characters have been converted
              return boost::optional<external_type>(boost::none);
            }
          } else {
            // treat as number
            try{
              i = boost::lexical_cast<int>(str);
            }
            catch (boost::bad_lexical_cast){
              return boost::optional<external_type>(boost::none);
            }
          }
          // return result
          return boost::optional<external_type>(i);
        }
        else
            return boost::optional<external_type>(boost::none);
    }

    // Converts a int to string
    boost::optional<internal_type> put_value(const external_type& i)
    {
      return boost::optional<internal_type>(std::to_string(i));
    }
};

/*  Specialize translator_between so that it uses our custom translator for
    int value types. Specialization must be in boost::property_tree
    namespace. */
namespace boost {
namespace property_tree {

template<typename Ch, typename Traits, typename Alloc> 
struct translator_between<std::basic_string< Ch, Traits, Alloc >, int>
{
    typedef hexTranslator type;
};

} // namespace property_tree
} // namespace boost

#endif
