#ifndef CODE_RP_RANDOM_H
#define CODE_RP_RANDOM_H

#include "Cache.h"
#include "RPolicy.h"

class RP_RANDOM : public RPolicy
{
  public:
    RP_RANDOM(CacheConfig config) : RPolicy(config)
    {
      name_ = "RP_RANDOM";
    };
    ~RP_RANDOM() override = default;
    
    unsigned insertionPosition(unsigned set, unsigned slice) override;
    InsertionPosition insertionPosition(CacheSet* set, unsigned slice) override;
    
    void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) override;
    void updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) override;

    void reset() override {};
    RPolicy *getCopy() override;
};

#endif //CODE_RP_RANDOM_H
