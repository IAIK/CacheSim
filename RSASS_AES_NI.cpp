#include <assert.h>
#include <math.h>

#include "RSASS_AES_NI.h"
#include "aes_ni.h"

typedef union {
    uint8_t u8[16];
    uint64_t u64[2];
}data128;

RSASS_AES_NI::RSASS_AES_NI(CacheConfig config, unsigned subsets, uint64_t key, int coverage_t) : RMapper(config, subsets, key)
{
  name_ = "SSAS_AES_NI"; //this is a problem with the copy constructor, isn't it -.-
  bits_needed_ =  log2(sets_) * subsets_ + coverage_t*subsets_;
  coverage_t_ = coverage_t;
}

CacheSet *RSASS_AES_NI::getIndices(uint64_t phys_addr, uint64_t secret)
{
  uint8_t hash[32];

  data128 plain;
  data128 key;
  plain.u64[0] = (phys_addr & ~((uint64_t)line_size_ - 1));
  plain.u64[1] = secret;
  key.u64[0] = key_;
  key.u64[1] = secret;

  calc_aes_ni(key.u8, plain.u8, hash);
  if (bits_needed_ > 128){
    key.u64[1] = key_^secret;
    calc_aes_ni(key.u8, plain.u8, &hash[16]);
  }
  
  //round 1
  for (unsigned subset = 0; subset < subsets_; subset++)
  {
    int size_b = line_bits_ - way_bits_ + coverage_t_;
    int byte_offset = subset*size_b / 8;
    int bit_offset = subset*size_b % 8;
    set_.index[subset] = (*((uint32_t *)(hash + byte_offset)) >> bit_offset) & ((1 << size_b) - 1);
  }

  //round 2
  for (unsigned subset = 0; subset < subsets_; subset++)
  {
    plain.u64[0] = set_.index[subset];
    plain.u64[1] = (uint64_t)subset << 56;
    key.u64[0] = key_;
    key.u64[1] = secret;
    calc_aes_ni(key.u8, plain.u8, hash);
    set_.index[subset] = *(uint32_t *)hash & ((lines_ - 1) >> way_bits_);
  }

  return &set_;
}

RMapper* RSASS_AES_NI::getCopy()
{
   return static_cast<RMapper*>(new RSASS_AES_NI(*this));
}

RSASS_AES_NI::~RSASS_AES_NI()
{
}
