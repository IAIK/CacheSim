#ifndef R_CAT_H
#define R_CAT_H

#include <cstdint>

#include "RMapper.h"

class RCAT : public RMapper
{
  public:
    RCAT(CacheConfig config, unsigned subsets, unsigned domains = 1);

    CacheSet *getIndices(uint64_t phys_addr, uint64_t domain) override;
    RMapper* getCopy() override;

    ~RCAT();
  private:
    unsigned domains_;
    unsigned domain_sets_;
};
#endif // R_CAT_H