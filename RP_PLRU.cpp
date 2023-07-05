#include "RP_PLRU.h"

unsigned RP_PLRU::insertionPosition(unsigned set, unsigned slice)
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
  int bit = 1 << (ways_ - 2); //start at top level decision bit
  for (unsigned i = 0; i < way_bits_; i++)
  {
    bool plru_bit = (plru_sets_[slice * sets_ + set] & bit) != 0;

    way <<= 1;

    way |= plru_bit;
    bit >>= plru_bit << i;

    bit >>= 1 << i;
  }

  return way;
}

InsertionPosition RP_PLRU::insertionPosition(CacheSet* set, unsigned slice)
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
  
  way = 0;
  int bit = 1 << (subset_ways_ - 2); //start at top level decision bit
  for (unsigned i = 0; i < subset_way_bits_; i++)
  {
    bool plru_bit = (plru_sets_[slice * subsets_ * sets_ + subset * sets_ + set->index[subset]] & bit) != 0;

    way <<= 1;

    way |= plru_bit;
    bit >>= plru_bit << i;

    bit >>= 1 << i;
  }

  return { subset, way };
}

void RP_PLRU::updateSet(unsigned set, unsigned slice, unsigned way, bool replacement)
{
  int bit = 1 << (ways_ - 2);
  for (unsigned i = 0; i < way_bits_; i++)
  {
    bool plru_bit = (way & (1 << (way_bits_ - (i + 1)))) != 0;

    //change bit away from path to current way
    if (plru_bit)
    {
      plru_sets_[slice * sets_ + set] &= ~bit;
      bit >>= 1 << i;
    }
    else
    {
      plru_sets_[slice * sets_ + set] |= bit;
    }

    bit >>= 1 << i;
  }
}

void RP_PLRU::updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement)
{
  int bit = 1 << (subset_ways_ - 2);
  for (unsigned i = 0; i < subset_way_bits_; i++)
  {
    bool plru_bit = (way & (1 << (subset_way_bits_ - (i + 1)))) != 0;

    //change bit away from path to current way
    if (plru_bit)
    {
      plru_sets_[slice * subsets_ * sets_ + subset * sets_ + set->index[subset]] &= ~bit;
      bit >>= 1 << i;
    }
    else
    {
      plru_sets_[slice * subsets_ * sets_ + subset * sets_ + set->index[subset]] |= bit;
    }

    bit >>= 1 << i;
  }
}

RPolicy* RP_PLRU::getCopy()
{
    RPolicy* copy = static_cast<RPolicy*>(new RP_PLRU(*this));
    ((RP_PLRU*)copy)->initCopy();
    return copy;
}
