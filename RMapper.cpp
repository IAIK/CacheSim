#include <assert.h>
#include <math.h>

#include "RMapper.h"

RMapper::RMapper(CacheConfig config, unsigned subsets, uint64_t key)
{
  line_size_bits_ = config.line_size_bits;
  line_size_ = 1 << line_size_bits_;

  line_bits_ = config.line_bits;
  lines_ = 1 << config.line_bits;
  size_ = config.slices * lines_ * line_size_;
  way_bits_ = config.way_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;
  sets_ = lines_ / ways_;
  subsets_ = subsets;
  subset_bits_ = log2(subsets_);
  subset_ways_ = ways_ / subsets_;
  key_ = key;
  name_ = "static";

  assert(!(ways_ % subsets_) && "number of ways can't be evenly split into subsets");
}

CacheSet *RMapper::getIndices(size_t phys_addr, uint64_t secret)
{
  uint32_t index = (phys_addr & ((sets_ - 1 ) << line_size_bits_)) >> line_size_bits_;
  
  for (unsigned i = 0; i < subsets_; i++)
    set_.index[i] = index;

  return &set_;
}

unsigned RMapper::getSlice(size_t phys_addr)
{
  //this is Intel's slicing function for 2^x cores
  int slice = 0;
  if (slices_ >= 2)
  {
    slice = __builtin_popcountll(phys_addr & SLICE0) % 2;
    if (slices_ >= 4)
    {
      slice |= (__builtin_popcountll(phys_addr & SLICE1) % 2) << 1;
      if (slices_ >= 8)
      {
        slice |= (__builtin_popcountll(phys_addr & SLICE2) % 2) << 2;
      }
    }
  }
  return slice;
}

RMapper* RMapper::getCopy()
{
   return new RMapper(*this);
}

RMapper::~RMapper()
{
}