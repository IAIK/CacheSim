#include <cstdlib>
#include <cstring>
#include <ctime>
#include <bitset>

#include "SACache.h"
#include "sha-256.h"
#include "RPolicy.h"
#include "assert.h"

#define DEBUG 0

SACache::SACache(CacheConfig config, RPolicy &policy) {
  line_size_bits_ = config.line_size_bits;
  line_size_ = 1 << line_size_bits_;

  line_bits_ = config.line_bits;
  lines_ = 1 << config.line_bits;
  size_ = config.slices * lines_ * line_size_;
  way_bits_ = config.way_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;

  sets_ = lines_ / ways_;
  name_ = "SA Cache";

  memory_ = (CacheLine *)malloc(slices_*lines_*sizeof(CacheLine));

  policy_ = policy.getCopy();
  policy_->setMemory(memory_);
  policy_name_ = policy_->getPolicyName();

  clearCache();

  srand(time(0));

  /*
  printf("\n-------------------------------------\n");
  printf("STANDARD CACHE PARAMETERS:\n");
  printf("size: %2.2fMB, lines: %u, sets: %u, ways: %u, slices: %u, linesize: %u\n", (float)size_ / 1024 / 1024, lines_, sets_, ways_, slices_, line_size_);
  printf("Eviction policy: %s\n", policy_ == RP_RANDOM ? "random" : policy_ == RP_LRU ? "LRU" : "pLRU");
  printf("-------------------------------------\n\n");*/

}

void SACache::clearCache()
{
  memset(memory_, 0, slices_*lines_*sizeof(CacheLine));
  memset(occupied_lines_, 0, sizeof(occupied_lines_));
  memset(cache_misses_, 0, sizeof(cache_misses_));
  memset(cache_hits_, 0, sizeof(cache_hits_));
  policy_->reset();
}

CLState SACache::isCached(size_t addr, size_t secret)
{
  CLState cl_state = MISS;
  size_t slice = getSlice(addr);
  unsigned set = getSetIndex(addr);

  size_t aligned_addr = (addr >> line_size_bits_) << line_size_bits_;
  for (uint32_t way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + set*ways_ + way].addr && memory_[slice*lines_ + set*ways_ + way].valid == 1)
    {
      cl_state = HIT;
      break;
    }
  }
  
  return cl_state;
}

AccessResult SACache::access(Access mem_access)
{
  AccessResult cl_state = {MISS, false, 0};

  unsigned slice = getSlice(mem_access.addr);
  assert(slice < slices_ && "Slice index too big");

  unsigned set = getSetIndex(mem_access.addr);

  size_t aligned_addr = (mem_access.addr >> line_size_bits_) << line_size_bits_;

  //look for address in set
  unsigned way;
  for (way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + set*ways_ + way].addr && memory_[slice*lines_ + set*ways_ + way].valid == 1)
    {
      cl_state.hit = HIT;

      break;
    }
  }

  if (cl_state.hit == HIT)
    cache_hits_[slice]++;
  else
    cache_misses_[slice]++;

  //select victim way & insert/replace
  if (cl_state.hit != HIT)
  {
    if (!policy_->isUMO())
      way = policy_->insertionPosition(set, slice);
    else
      way = policy_->updateAndInsert(set, slice);

    //return evicted address so we can do inclusive cache hierarchies
    if (memory_[slice * lines_ + set * ways_ + way].valid && cl_state.hit == MISS)
    {
      cl_state.evicted = true;
      cl_state.evicted_addr = memory_[slice * lines_ + set * ways_ + way].addr;
    }

    //replace victim line
    memory_[slice * lines_ + set * ways_ + way].addr = aligned_addr;
  }

  policy_->updateSet(set, slice, way, cl_state.hit == MISS);

  //set valid
  memory_[slice * lines_ + set * ways_ + way].valid = true;

  /*for (int i = 0; i < lines_; i++)
      printf("%d ", memory_[i].valid ? memory_[i].time : 9);
  printf(" - %d", mem_access.addr);
  printf("\n");*/

  //mark used
  if (!memory_[slice * lines_ + set * ways_ + way].used)
  {
    occupied_lines_[slice]++;
    memory_[slice * lines_ + set * ways_ + way].used = true;
  }

 /*#if DEBUG
  switch (policy_)
  {
    case RP_BIP:
    case RP_LRU:
    {
      printf("addr: %8lx, aligned: %8lx, slice: %2u,            set: %5u, way: %2u, ages: ", mem_access.addr, aligned_addr, slice, set, way);
      for (unsigned i = 0; i < ways_; i++)
        printf("%d%c ", memory_[slice*lines_ + set*ways_ + i].time, memory_[slice*lines_ + set*ways_ + i].valid ? ' ' : 'i');
      printf("\n");
      break;
    }

    case RP_PLRU:
    {
      std::bitset<15> bits(plru_sets_[slice*sets_ + set]);
      printf("addr: %8lx, aligned: %8lx, slice: %2u,            set: %5u, way: %2u, mask: ", mem_access.addr, aligned_addr, slice, set, way);
      printf("%s\n", bits.to_string().c_str());
      break;
    }

    default:
      break;
  }
#endif */

  return cl_state;
}

void SACache::resetUsage(int slice)
{
  for (uint32_t i = 0; i < slices_*lines_; i++)
    memory_[i].used = 0;
  for (unsigned i = 0; i < slices_; i++)
    occupied_lines_[i] = 0;
}

void SACache::flush(Access mem_access)
{
  size_t slice = getSlice(mem_access.addr);
  unsigned set = getSetIndex(mem_access.addr);

  size_t aligned_addr = (mem_access.addr >> line_size_bits_) << line_size_bits_;
  for (uint32_t way = 0; way < ways_; ++way)
  {
    if (aligned_addr == memory_[slice*lines_ + set*ways_ + way].addr && memory_[slice*lines_ + set*ways_ + way].valid == 1)
    {
      memory_[slice*lines_ + set*ways_ + way].valid = false;
      break;
    }
  }
}

SACache::~SACache()
{
  //printf("standard cache destroyed\n");
  free(memory_);
  delete policy_;
}