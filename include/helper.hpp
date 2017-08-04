#ifndef CADIDAQ_HELPER_H
#define CADIDAQ_HELPER_H

#include <bitset>
#include <iterator>  // distance

#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> // boost::starts_with


/** converts a vector of optional<bool> into a uint32 bit mask.
    defaults to bit=0 if corresponding optional not set.
    groups parameter allows to group the bits with any bit in the group being 1 leading to the resulting group's bit being 1
    blocksize parameter allows to switch bits in blocks of the given number of bits.
*/
inline uint32_t vec2Mask(std::vector<boost::optional<bool>>& vec, uint ngroups, uint blocksize = 1){
  std::bitset<32> mask(0);
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    // set value if set and 'true' (else it stays 0)
    if (*it && **it) {
      // create a bit pattern for the channel/group the channel belongs to with size of blocksize
      uint32_t shift = (((uint32_t)1 << blocksize) - 1) << index/(ngroups*blocksize);
      // set that pattern (applied multiple times if groupsize>1 and remains 'true' if one of the group's members was)
      mask |= shift;
    }
  }
  return mask.to_ulong();
}

/// fills the given bit mask into a vector of boost::optional<bool>
inline  void mask2Vec(boost::optional<uint32_t> mask, std::vector<boost::optional<bool>>& vec, uint ngroups = 1){
  if (!mask){
    // invalid argument -> set vector elements to boost::none
    for (auto it = vec.begin(); it != vec.end(); ++it)
      *it = boost::none;
    return;
  }
  std::bitset<32> bits(*mask);
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    uint group = index/ngroups;
    // set vector entry
    *it = bits[group];
  }
}

/// counts the number of 'true' values in a vector of boost::optional<bool> (optionally limited to an index range startidx--stopidx)
inline int countTrue(std::vector<boost::optional<bool>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  int nTrue = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (index >= startidx && index < stopidx && *it && **it) {
      nTrue++;
    }
  }
  return nTrue;
}

/// counts the number of set values in a vector of boost::optional<bool> (optionally limited to an index range startidx--stopidx)
template <typename T>
inline int countSet(std::vector<boost::optional<T>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  int nSet = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (index >= startidx && index < stopidx && *it) {
      nSet++;
    }
  }
  return nSet;
}

/// returns the first set value in a range of indexes in a vector of boost::optional _or_ returns a boost::optional set to boost::none if no element is set
template <typename T>
inline boost::optional<T> getFirstSetValue(std::vector<boost::optional<T>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  // find first defined value in the idx range
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (index >= startidx && index < stopidx && (*it)){
      return *it;
    }
  }
  return boost::optional<T>(boost::none);
}

/// tests if all values in a vector of boost::optional are set to some identical value _or_ all are undefined (optionally limited to an index range startidx--stopidx)
template <typename T>
inline bool allValuesSame(std::vector<boost::optional<T>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  // get value of first defined element
  boost::optional<T> value = getFirstSetValue(vec, startidx, stopidx);
  // compare against all values
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (index >= startidx && index < stopidx && *it && (*it != value))
      return false;
  }
  return true;
}

/// tests if all values in a vector of boost::optional are set to any defined value (optionally limited to an index range startidx--stopidx)
template <typename T>
inline bool allValuesSet(std::vector<boost::optional<T>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (!*it && index >= startidx && index < stopidx)
      return false;
  }
  return true;
}

/// tests if all values in a vector of boost::optional are unset/undefined (optionally limited to an index range startidx--stopidx)
template <typename T>
inline bool noValuesSet(std::vector<boost::optional<T>>& vec, size_t startidx = 0, size_t stopidx = std::numeric_limits<std::size_t>::max()){
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    if (*it && index >= startidx && index < stopidx)
      return false;
  }
  return true;
}

/// finds the common root-prefix for the enums in the given boost::bimap (e.g. "CAEN_DGTZ_")
template <class CAEN_ENUM>
inline std::string findEnumRoot(boost::bimap< std::string, CAEN_ENUM >& map){
  std::string root = map.left.begin()->first;
  for(const auto& item : map.left){
    if (root.length() > item.first.length()){
      root.erase(item.first.length(), std::string::npos);
    }
    for(int i = 0; i < root.length(); ++i){
      if (root[i] != item.first[i]){
        root.erase(i, std::string::npos);
        break;
      }
    }
  }
  return root;
}

/** splits a comma-separated list of values and ranges into
    individual values, returns these as vector */
inline std::vector<int> expandRange(std::string range){
  // clean input of spaces
  boost::erase_all(range," ");
  // helper vars
  std::vector<std::string> strs,r;
  std::vector<int> v;
  int low,high,i;
  // split string
  boost::split(strs,range,boost::is_any_of(","));
  // expand values
  for (auto it:strs){
    boost::split(r,it,boost::is_any_of("-"));
    auto x = r.begin();
    low = high =boost::lexical_cast<int>(r[0]);
    x++;
    if(x!=r.end())
      high = boost::lexical_cast<int>(r[1]);
    for(i=low;i<=high;++i)
      v.push_back(i);
  }
  return v;
}

/// converts a string to a hex value
inline boost::optional<uint32_t> str2hex(std::string str){
  // clean input of spaces
  boost::erase_all(str," ");
  if (!str.empty()){
      uint32_t i = 0;
      if (boost::istarts_with(str, "0x")){
        // treat as hex
        std::stringstream ss(str);
        ss >> std::hex >> i;
        return boost::optional<uint32_t>(i);
      } else {
        // treat as number
        try{
          i = boost::lexical_cast<uint32_t>(str);
        }
        catch (boost::bad_lexical_cast){
          return boost::optional<uint32_t>(boost::none);
        }
      }
      // return result
      return boost::optional<uint32_t>(i);
    }
  else
    return boost::optional<uint32_t>(boost::none);
}

/// convert a uint32_t into a str in hex notation
inline std::string hex2str(uint32_t i){
  std::stringstream s;
  s << std::hex << std::showbase << i;
  return s.str();
}

/// identifies last element in an iteration 
template <typename Iter, typename Cont>
inline bool is_last(Iter iter, const Cont& cont){
  return (iter != cont.end()) && (next(iter) == cont.end());
}


#endif
