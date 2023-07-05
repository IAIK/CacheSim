//
// Created by arn on 7/17/21.
//
#ifndef CODE_RPOLICY_H
#define CODE_RPOLICY_H

#include <string>
#include "math.h"
#include "Cache.h"
#include "CacheMemory.h"

using std::string;

struct InsertionPosition
{
  unsigned subset;
  unsigned way;
};

class RPolicy
{
  public:
    RPolicy(CacheConfig config)
    {
      ways_ = 1 << config.way_bits;
      lines_ = 1 << config.line_bits;
      sets_ = lines_ / ways_;
      subsets_ = 1;
      subset_ways_ = ways_;
    }
    virtual ~RPolicy() = default;
    
    virtual unsigned insertionPosition(unsigned set, unsigned slice) = 0;
    virtual InsertionPosition insertionPosition(CacheSet* set, unsigned slice) = 0;

    virtual unsigned updateAndInsert(unsigned set, unsigned slice) { return 0; }
    virtual InsertionPosition updateAndInsert(CacheLine** line, CacheSet* set, unsigned slice) { return { 0, 0 }; };

    virtual void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) = 0;
    virtual void updateSet(CacheLine &line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) = 0;

    string getPolicyName() { return name_; };
    virtual void setSubsets(unsigned subsets) 
    { 
      if (subsets == 1)
        return;
      subsets_ = subsets;
      subset_ways_ = ways_ / subsets_;
    };
    void setMemory(CacheLine *memory) { memory_ = memory; };
    void setCMemory(CacheMemory *c_memory) { c_memory_ = c_memory; };
    bool isUMO() { return umo_; };
    virtual void reset() = 0;
    virtual RPolicy* getCopy() = 0;

  protected:
    string name_;
    unsigned ways_;
    unsigned subsets_;
    unsigned subset_ways_;
    unsigned lines_;
    unsigned sets_;
    // updateOnMissOnly -> QLRU exclusive
    bool umo_ = false;
    CacheLine *memory_;
    CacheMemory *c_memory_{};
};

#endif //CODE_RPOLICY_H
