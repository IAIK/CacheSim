#ifndef CODE_RP_LRU_H
#define CODE_RP_LRU_H

#include "RPolicy.h"

class RP_LRU : public RPolicy
{
  public:
    RP_LRU(CacheConfig config) : RPolicy(config)
    {
        name_ = "RP_LRU";
    };
    ~RP_LRU() override = default;

    unsigned insertionPosition(unsigned set, unsigned slice) override;
    InsertionPosition insertionPosition(CacheSet* set, unsigned slice) override;

    void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) override;
    void updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) override;

    void reset() override {};
    RPolicy* getCopy() override;
};


#endif //CODE_RP_LRU_H
