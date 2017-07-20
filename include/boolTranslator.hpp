// provides a translator for booleans in Boost property trees
// to parse e.g. "true" as 1
// code adopted from https://stackoverflow.com/a/9754025

#ifndef CADIDAQ_boolTranslator_hpp
#define CADIDAQ_boolTranslator_hpp

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/algorithm/string/predicate.hpp>

// Custom translator for bool (only supports std::string)
struct BoolTranslator
{
    typedef std::string internal_type;
    typedef bool        external_type;

    // Converts a string to bool
    boost::optional<external_type> get_value(const internal_type& str)
    {
        if (!str.empty())
        {
            using boost::algorithm::iequals;

            if (iequals(str, "true") || str == "t" || iequals(str, "yes") || iequals(str, "on") || str == "1")
              return boost::optional<external_type>(true);
            else if (iequals(str, "false") || str == "f" || iequals(str, "no") || iequals(str, "off") || str == "0")
              return boost::optional<external_type>(false);
            else
              throw boost::property_tree::ptree_bad_data("Cannot translate string to boolean value", str);
        }
        else
            return boost::optional<external_type>(boost::none);
    }

    // Converts a bool to string
    boost::optional<internal_type> put_value(const external_type& b)
    {
        return boost::optional<internal_type>(b ? "true" : "false");
    }
};

/*  Specialize translator_between so that it uses our custom translator for
    bool value types. Specialization must be in boost::property_tree
    namespace. */
namespace boost {
namespace property_tree {

template<typename Ch, typename Traits, typename Alloc> 
struct translator_between<std::basic_string< Ch, Traits, Alloc >, bool>
{
    typedef BoolTranslator type;
};

} // namespace property_tree
} // namespace boost

#endif
