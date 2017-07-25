// settings.hpp
#ifndef CADIDAQ_HELPER_H
#define CADIDAQ_HELPER_H

#include <boost/optional.hpp>
#include <bitset>
#include <iterator>  // distance

/* converts a vector of optional<bool> into a uint32 bit mask.
   defaults to 0 if optional not set */
uint32_t vec2Mask(std::vector<boost::optional<bool>>& vec, uint groupsize = 1){
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

void mask2Vec(uint32_t mask, std::vector<boost::optional<bool>>& vec, uint groupsize = 1){
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

#endif
