#include <assert.h>
#include <math.h>

#include "RAES_NI.h"
#include "aes_ni.h"

typedef union {
    uint8_t u8[16];
    uint64_t u64[2];
}data128;

RAES_NI::RAES_NI(CacheConfig config, unsigned subsets, uint64_t key) : RMapper(config, subsets, key)
{
  name_ = "AES_NI";
  bits_needed_ =  log2(sets_) * subsets_;
}

CacheSet *RAES_NI::getIndices(uint64_t phys_addr, uint64_t secret)
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
    key.u64[1] = key_;
    calc_aes_ni(key.u8, plain.u8, &hash[16]);
  }
  
  unsigned subset;
  for (subset = 0; subset < subsets_; subset++)
  {
    int byte_offset = subset*(line_bits_ - way_bits_) / 8;
    int bit_offset = subset*(line_bits_ - way_bits_) % 8;
    set_.index[subset] = (*((uint32_t *)(hash + byte_offset)) >> bit_offset) & ((lines_ - 1) >> way_bits_);
  }

  return &set_;
}

RMapper* RAES_NI::getCopy()
{
   return static_cast<RMapper*>(new RAES_NI(*this));
}

RAES_NI::~RAES_NI()
{
}
