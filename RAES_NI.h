#ifndef R_AES_NI_H
#define R_AES_NI_H

#include <cstdint>

#include "RMapper.h"

class RAES_NI : public RMapper
{
  public:
    RAES_NI(CacheConfig config, unsigned P, uint64_t key = 0);

    CacheSet *getIndices(uint64_t phys_addr, uint64_t secret) override;
    RMapper* getCopy() override;

    ~RAES_NI();
  
  public:
    unsigned bits_needed_;

};
#endif // R_AES_NI_H