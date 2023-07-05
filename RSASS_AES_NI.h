#ifndef R_SSAS_AES_NI_H
#define R_SSAS_AES_NI_H

#include <cstdint>

#include "RMapper.h"

class RSASS_AES_NI : public RMapper
{
  public:
    RSASS_AES_NI(CacheConfig config, unsigned P, uint64_t key = 0, int coverage_t = 0);

    CacheSet *getIndices(uint64_t phys_addr, uint64_t secret) override;
    RMapper* getCopy() override;

    ~RSASS_AES_NI();
  
  public:
    unsigned bits_needed_;
    int coverage_t_;

};
#endif // R_SSAS_AES_NI_H