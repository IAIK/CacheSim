#include <assert.h>

#include "RSHA256.h"
#include "sha-256.h"

RSHA256::RSHA256(CacheConfig config, unsigned subsets, uint64_t key) : RMapper(config, subsets, key)
{
  name_ = "SHA256";
}

CacheSet *RSHA256::getIndices(uint64_t phys_addr, uint64_t secret)
{
  uint8_t hash[32];

  uint64_t hash_input[2];
  hash_input[0] = (phys_addr & ~((uint64_t)line_size_ - 1));
  hash_input[1] = key_ ^ secret;

  calc_sha_256(hash, hash_input, 16);
  
  unsigned subset;
  for (subset = 0; subset < subsets_; subset++)
  {
    int byte_offset = subset*(line_bits_ - way_bits_) / 8;
    int bit_offset = subset*(line_bits_ - way_bits_) % 8;
    set_.index[subset] = (*((uint32_t *)(hash + byte_offset)) >> bit_offset) & ((lines_ - 1) >> way_bits_);
  }

  return &set_;
}

RMapper* RSHA256::getCopy()
{
   return static_cast<RMapper*>(new RSHA256(*this));
}

RSHA256::~RSHA256()
{
}