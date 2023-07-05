#ifndef CODE_RP_PLRU_H
#define CODE_RP_PLRU_H

#include "RPolicy.h"

#include <cstring>

class RP_PLRU : public RPolicy
{
  public:
    RP_PLRU(CacheConfig config) : RPolicy(config)
    {
      way_bits_ = config.way_bits;
      subset_way_bits_ = way_bits_;
      slices_ = config.slices;
      //memory allocated in initCopy(), original version does not need this
      plru_sets_ = NULL;
      name_ = "RP_PLRU";
    };
    ~RP_PLRU() override { 
      if (plru_sets_ != NULL) 
        free(plru_sets_); 
    };
    
    unsigned insertionPosition(unsigned set, unsigned slice) override;
    InsertionPosition insertionPosition(CacheSet* set, unsigned slice) override;

    void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) override;
    void updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) override;
    
    /* Sets the number of subsets in the cache configuration, this has to be done manually in the *Cache.cpp */
    void setSubsets(unsigned subsets) override
    {
      if (subsets == 1)
        return;

      subsets_ = subsets;
      subset_ways_ = ways_ / subsets_;
      subset_way_bits_ = log2(subset_ways_);
      plru_sets_ = (uint64_t*) realloc(plru_sets_, sets_ * subsets_ * slices_ * sizeof(*plru_sets_));
    };

    void reset() override { memset(plru_sets_, 0, sets_*subsets_*slices_*sizeof(*plru_sets_)); };
    RPolicy* getCopy() override;
    void initCopy() {
      plru_sets_ = (uint64_t*)malloc(sets_ * subsets_ * slices_ * sizeof(*plru_sets_)); 
    };

  private:
    unsigned way_bits_;
    unsigned subset_way_bits_;
    unsigned slices_;
    uint64_t *plru_sets_;
};


#endif //CODE_RP_PLRU_H
