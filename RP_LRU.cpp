#include "RP_LRU.h"

unsigned RP_LRU::insertionPosition(unsigned set, unsigned slice)
{
  // check for invalid lines
  for (unsigned i = 0; i < ways_; i++)
  {
    if (!memory_[slice * lines_ + set * ways_ + i].valid)
    {
      return i;
    }
  }

  unsigned way = 0;
  for (unsigned i = 0; i < ways_; i++)
  {
    if (memory_[slice * lines_ + set * ways_ + i].time == 0)
    {
      way = i;
      break;
    }
  }

  return way;
}

InsertionPosition RP_LRU::insertionPosition(CacheSet* set, unsigned slice)
{
  unsigned subset, way;
  for (subset = 0; subset < subsets_; subset++)
  {
    for (way = 0; way < subset_ways_; way++)
    {
      if (!(*c_memory_)(slice, subset, set, way).valid)
      {
        return { subset, way };
      }
    }
  }

  subset = 0;
  if (subsets_ != 1)
    subset = rand() % subsets_;

  for (way = 0; way < subset_ways_; way++)
  {
    if ((*c_memory_)(slice, subset, set, way).time == 0)
    {
      break;
    }
  }

  return { subset, way };
}

void RP_LRU::updateSet(unsigned set, unsigned slice, unsigned way, bool replacement)
{
  int old_pos = memory_[slice * lines_ + set * ways_ + way].time;

  for (unsigned i = 0; i < ways_; i++)
  {
    if (memory_[slice * lines_ + set * ways_ + i].time > old_pos)
      memory_[slice * lines_ + set * ways_ + i].time--;
  }
  memory_[slice * lines_ + set * ways_ + way].time = ways_ - 1;
}

void RP_LRU::updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement)
{
  int old_pos = line.time;
  for (unsigned i = 0; i < subset_ways_; i++)
  {
    if ((*c_memory_)(slice, subset, set, i).time > old_pos)
      (*c_memory_)(slice, subset, set, i).time--;
  }
  line.time = subset_ways_ - 1;
}

RPolicy* RP_LRU::getCopy()
{
    return static_cast<RPolicy*>(new RP_LRU(*this));
}
