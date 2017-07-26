#ifndef CADIDAQ_HELPER_H
#define CADIDAQ_HELPER_H

#include <bitset>
#include <iterator>  // distance

#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> // boost::starts_with

/** converts a vector of optional<bool> into a uint32 bit mask.
   defaults to 0 if optional not set */
inline uint32_t vec2Mask(std::vector<boost::optional<bool>>& vec, uint groupsize = 1){
  std::bitset<32> mask(0);
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    // set value if set and 'true' (else it stays 0)
    if (*it && **it) {
      uint groupidx = index%groupsize;
      // treat "special case" of all channels belonging to the same group (e.g. no groups, just channels)
      if (groupsize == 1)
        groupidx = index;
      // create a bit pattern for the group the channel belongs to
      uint32_t shift = (((uint32_t)1 << groupsize) - 1) << groupidx;
      // set that pattern (applied multiple times if groupsize>1 and remains 'true' if one of the group's members was)
      mask |= shift;
    }
  }
  return mask.to_ulong();
}

/// fills the given bit mask into a vector of boost::optional<bool>
inline  void mask2Vec(uint32_t mask, std::vector<boost::optional<bool>>& vec, uint groupsize = 1){
  std::bitset<32> bits(mask);
  // TODO: implement treatment of groupsizes > 1
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    auto index = std::distance(vec.begin(), it);
    uint groupidx = index%groupsize;
    // treat "special case" of all channels belonging to the same group (e.g. no groups, just channels)
    if (groupsize == 1)
      groupidx = index;
    // set vector entry
    *it = bits[groupidx];
  }
}

/// counts the number of 'true' values in a vector of boost::optional<bool>
inline int countTrue(std::vector<boost::optional<bool>>& vec){
  int nTrue = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (*it && **it) {
      nTrue++;
    }
  }
  return nTrue;
}

/// tests if all values in a vector of boost::optional are set to a value
template <typename T>
inline bool allValuesSet(std::vector<boost::optional<T>>& vec){
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (!*it)
      return false;
  }
  return true;
}

/// tests if all values in a vector of boost::optional are unset/undefined
template <typename T>
inline bool noValuesSet(std::vector<boost::optional<T>>& vec){
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (*it)
      return false;
  }
  return true;
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

// identifies last element in an iteration 
template <typename Iter, typename Cont>
inline bool is_last(Iter iter, const Cont& cont){
  return (iter != cont.end()) && (next(iter) == cont.end());
}


#endif
