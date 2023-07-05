#include <assert.h>

#include "CacheMemory.h"
#include "Cache.h"

CacheMemory::CacheMemory(CacheConfig config, unsigned subsets)
{
  lines_ = 1 << config.line_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;
  sets_ = lines_ / ways_;
  subsets_ = subsets;
  subset_ways_ = ways_ / subsets_;
  memory_ = (CacheLine *)malloc(slices_ * lines_ * sizeof(CacheLine));
  external_memory_ = false;
}

CacheMemory::CacheMemory(CacheConfig config, unsigned subsets, CacheLine *memory)
{
  lines_ = 1 << config.line_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;
  sets_ = lines_ / ways_;
  subsets_ = subsets;
  subset_ways_ = ways_ / subsets_;
  memory_ = memory;
  external_memory_ = true;
}

CacheLine& CacheMemory::operator()(unsigned slice, unsigned subset, unsigned set, unsigned way)
{
  uint32_t index = slice * lines_ + subset * sets_ * subset_ways_ + set * subset_ways_ + way;

  assert(way < subset_ways_ && "way oob");
  assert(set < sets_ && "set oob");
  assert(subset < subsets_ && "subset oob");
  assert(slice < slices_ && "slice oob");
  assert(index < slices_ * lines_ && "oob access to cache memory");
  return memory_[index];
}

CacheLine& CacheMemory::operator()(unsigned slice, unsigned subset, CacheSet *set, unsigned way)
{
  uint32_t set_index = set->index[subset];
  return (*this)(slice, subset, set_index, way);
}

CacheLine& CacheMemory::operator()(unsigned slice, unsigned set, unsigned way)
{
  //calling like this assumes subsets = 1 -> subset_ways_ = ways_
  return (*this)(slice, 0, set, way);
}

CacheMemory::~CacheMemory()
{
  if (!external_memory_)
    free(memory_);
}