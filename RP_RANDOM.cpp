#include "stdlib.h"
#include "RP_RANDOM.h"

unsigned RP_RANDOM::insertionPosition(unsigned set, unsigned slice)
{
  // check for invalid lines
  for (unsigned i = 0; i < ways_; i++)
  {
    if (!memory_[slice * lines_ + set * ways_ + i].valid)
    {
      return i;
    }
  }

	return rand() % ways_;
}

InsertionPosition RP_RANDOM::insertionPosition(CacheSet* set, unsigned slice)
{
  unsigned subset;
  for (subset = 0; subset < subsets_; subset++)
  {
    for (unsigned way = 0; way < subset_ways_; way++)
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

  return { subset, rand() % subset_ways_ };
}

// TODO SAVE SOME CYCLES BY NOT CALLING THESE IF POLICY ISNT RANDOM
void RP_RANDOM::updateSet(unsigned set, unsigned slice, unsigned way, bool replacement)
{}

void RP_RANDOM::updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement)
{}

RPolicy* RP_RANDOM::getCopy()
{
	return static_cast<RPolicy*>(new RP_RANDOM(*this));
}
