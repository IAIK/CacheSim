#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "ScatterCache.h"
#include "sha-256.h"

#define DEBUG_SET 0


ScatterCache::ScatterCache(CacheConfig config, SCVersion version)
{
  version_ = version;
  line_size_bits_ = config.line_size_bits;
  line_size_ = 1 << line_size_bits_;
  line_bits_ = config.line_bits;
  lines_ = 1 << config.line_bits;
  size_ = config.slices * lines_ * line_size_;
  way_bits_ = config.way_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;
  name_ = "Scattercache";
  policy_name_ = "random";
  
  memory_ = (CacheLine *)malloc(slices_*lines_*sizeof(CacheLine));
  clearCache();

  srand(time(0));
  
/*
  printf("\n-------------------------------------\n");
  printf("SCATTERCACHE PARAMETERS:\n");
  printf("size: %2.2fMB, lines: %u, ways: %u, slices: %u, linesize: %lu\n", (float)size_ / 1024 / 1024, lines_, ways_, slices_, line_size_);
  printf("-------------------------------------\n\n");
*/

}

ScatterCache::ScatterCache(CacheConfig config, SCVersion version, float noise) : ScatterCache(config, version) 
{
  noisy_ = true;
  noise_ = noise*RAND_MAX;
}

void ScatterCache::clearCache()
{
  memset(memory_, 0, slices_*lines_*sizeof(CacheLine));
  memset(occupied_lines_, 0, sizeof(occupied_lines_));
  memset(cache_misses_, 0, sizeof(cache_misses_));
  memset(cache_hits_, 0, sizeof(cache_hits_));
}

CLState ScatterCache::isCached(size_t addr, size_t secret)
{
  CLState cl_state = MISS;
  size_t slice = getSlice(addr);
  getScatterSet(secret, addr);

  size_t aligned_addr = (addr >> line_size_bits_) << line_size_bits_;
  for (uint32_t way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr && memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid == 1)
    {
      cl_state = HIT;
      break;
    }
  }
  return cl_state;
}

void ScatterCache::randomAccess()
{
  static uint64_t i = 0;
  extAccess((i++)*64, false, 324646576, true, 0, 0, 0, true);
}

AccessResult ScatterCache::access(Access mem_access)
{
  return extAccess(mem_access.addr, mem_access.write, mem_access.secret, true, 0, 0, 0, false);
}

void ScatterCache::resetUsage(int slice)
{
  for (uint32_t i = 0; i < slices_*lines_; i++)
    memory_[i].used = 0;
  for (unsigned i = 0; i < slices_; i++)
    occupied_lines_[i] = 0;
}

void ScatterCache::flush(Access mem_access)
{
  size_t slice = getSlice(mem_access.addr);
  getScatterSet(mem_access.secret, mem_access.addr);

  size_t aligned_addr = (mem_access.addr >> line_size_bits_) << line_size_bits_;
  for (uint32_t way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr && memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid == 1)
    {
      memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid = false;
      break;
    }
  }
}

CacheSet ScatterCache::getScatterSetV1(uint64_t secret, size_t phys_addr)
{
  uint8_t hash[32];

  int offset = 0;

  while (1)
  {

    uint64_t hash_input[2];
    hash_input[0] = (phys_addr & ~(line_size_ - 1));
    hash_input[1] = secret;

    calc_sha_256(hash, hash_input, 16);
    
    unsigned way;
    for (way = 0; way < ways_; way++)
    {
      
      int byte_offset = way*(line_bits_ - way_bits_) / 8;
      int bit_offset = way*(line_bits_ - way_bits_) % 8;
      scatter_set_.index[way] = (*((uint32_t *)(hash + byte_offset)) >> bit_offset) & ((lines_ - 1) >> way_bits_);

      /* can't have duplicates when the cache is segmented into [way] blocks
      //check for duplicates
      int i;
      for (i = 0; i < way; i++)
        if (set.index[way] == set.index[i])
          break;
      if (i != way)
        break;*/
    }
    if (way == ways_)
      break;
    offset++;
  }

  return scatter_set_;
}

CacheSet ScatterCache::getScatterSetV2(uint64_t secret, size_t phys_addr)
{
  uint8_t hash[32];

  const uint64_t index_mask = (1 << (line_bits_ - way_bits_ + line_size_bits_)) - 1;
  uint64_t index = (phys_addr & index_mask) >> line_size_bits_;

  uint64_t hash_input[3];
  hash_input[0] = secret;
  hash_input[1] = phys_addr >> (line_bits_ - way_bits_ + line_size_bits_);

  calc_sha_256(hash, &hash_input, 16);

  unsigned way;
  for (way = 0; way < ways_; way++)
  {
    
    int byte_offset = way*(line_bits_ - way_bits_) / 8;
    int bit_offset = way*(line_bits_ - way_bits_) % 8;
    uint32_t way_hash = (*((uint32_t *)(hash + byte_offset)) >> bit_offset) & ((lines_ - 1) >> way_bits_);

    scatter_set_.index[way] = (index ^ way_hash) & ((1 << (line_bits_ - way_bits_)) - 1);

  }
  
  /*unsigned way;
  for (way = 0; way < ways_; way++)
  {
    hash_input[2] = way;
    calc_sha_256(hash, &hash_input, 24);
    //printf("%p\n", ((uint32_t *)hash)[0]);
    set.index[way] = (index ^ ((uint32_t *)hash)[0]) & ((1 << (line_bits_ - way_bits_)) - 1);
  }*/

  return scatter_set_;
}

CacheSet ScatterCache::getScatterSet(uint64_t secret, size_t phys_addr)
{
  if (version_ == SC_V1)
    return getScatterSetV1(secret, phys_addr);
  else
    return getScatterSetV2(secret, phys_addr);
}

AccessResult ScatterCache::extAccess(size_t addr, bool write, size_t secret, bool quiet, size_t* test_set, uint32_t test_set_size, bool* test_hit, bool no_noise)
{
  if (noisy_ && !no_noise && rand() < noise_)
    randomAccess();

  AccessResult cl_state = {MISS, false, 0};
  unsigned slice = getSlice(addr);
  if (slice >= slices_) exit(-1);

  getScatterSet(secret, addr);

  size_t aligned_addr = (addr >> line_size_bits_) << line_size_bits_;
  
  uint32_t way;
  for (way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr && memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid == 1)
    {
      cl_state.hit = HIT;
      break;
    }
  }
  
  if (cl_state.hit == HIT)
    cache_hits_[slice]++;
  else
    cache_misses_[slice]++;

  //replacement policy = random
  if (cl_state.hit != HIT)
  {
    way = rand() % ways_;
    //prefer free cachelines first
    for (uint32_t i = 0; i < ways_; ++i)
    {
      if (!memory_[slice*lines_ + i*lines_/ways_ + scatter_set_.index[i]].valid)
      {
        way = i;
        break;
      }
    }
  }

  //check if an address of a test set is being overwritten
  if (test_set_size != 0)
  {
    *test_hit = false;
    for (unsigned i = 0; i < test_set_size; i++)
    {
      size_t aligned_test_addr = (test_set[i] >> line_bits_) << line_bits_;
      if (memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr == aligned_test_addr)
      {
        *test_hit = true;
        break;
      }
    }
  }

  //return evicted address so we can do inclusive cache hierarchies
  if (memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid && cl_state.hit == MISS)
  {
    cl_state.evicted = true;
    cl_state.evicted_addr = memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr;
  }
  
  memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].addr = aligned_addr;

#if DEBUG_SET
  printf("set: ");
  for (unsigned i = 0; i < ways_; i++)
    printf("%u, ", scatter_set_.index[i]);
  printf("\n");
#endif

  if (!quiet)
  {
    printf("%s %s %18p (slc=%u, set=%5u, way=%2u)\n", write ? "write" : " read", cl_state.hit == HIT ? "hit " : "miss", (void*)addr, slice, scatter_set_.index[way], way);

    printf("set: ");
    for (unsigned i = 0; i < ways_; i++)
      printf("%u, ", scatter_set_.index[i]);
    printf("\n");
  }

  if (!memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].used)
  {
    occupied_lines_[slice]++;
    memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].used = true;
  }



  //cache[slice][scatter_set.index[way]].time = time;
  memory_[slice*lines_ + way*lines_/ways_ + scatter_set_.index[way]].valid = true;

  return cl_state;
}

ScatterCache::~ScatterCache()
{
  //printf("scattercache destroyed\n");
  free(memory_);
}
