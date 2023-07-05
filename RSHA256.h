#ifndef R_SHA256_H
#define R_SHA256_H

#include <cstdint>

#include "RMapper.h"

class RSHA256 : public RMapper
{
  public:
    RSHA256(CacheConfig config, unsigned P, uint64_t key = 0);

    CacheSet *getIndices(uint64_t phys_addr, uint64_t secret) override;
    RMapper* getCopy() override;

    ~RSHA256();
  
  public:

};
#endif // R_SHA256_H