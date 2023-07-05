#ifndef CODE_RP_BIP_H
#define CODE_RP_BIP_H

#include "RPolicy.h"

class RP_BIP : public RPolicy
{
  public:
    RP_BIP(CacheConfig config) : RPolicy(config) 
    {
      name_ = "RP_BIP";
    };
    ~RP_BIP() override = default;

    unsigned insertionPosition(unsigned set, unsigned slice) override;
    InsertionPosition insertionPosition(CacheSet* set, unsigned slice) override;

    void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) override;
    void updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) override;

    void reset() override {};
    RPolicy* getCopy() override;
};


#endif //CODE_RP_BIP_H
