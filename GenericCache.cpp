#include <assert.h>
#include <string.h>
#include <math.h>
#include <bitset>

#include "GenericCache.h"
#include "CacheMemory.h"
#include "sha-256.h"

#define DEBUG 0
#define DEBUG_SET 0

GenericCache::GenericCache(CacheConfig config, RPolicy &policy, RMapper &R, unsigned subsets)
{
  line_size_bits_ = config.line_size_bits;
  line_size_ = 1 << line_size_bits_;
  line_bits_ = config.line_bits;
  lines_ = 1 << config.line_bits;
  size_ = config.slices * lines_ * line_size_;
  way_bits_ = config.way_bits;
  ways_ = 1 << config.way_bits;
  slices_ = config.slices;
  sets_ = lines_ / ways_;
  subsets_ = subsets;
  subset_ways_ = ways_ / subsets_;
  subset_way_bits_ = log2(subset_ways_);

  assert(!(ways_ % subsets_) && "number of ways can't be evenly split into subsets");

  name_ = "Generic Cache - " + R.name_;

  memory_ = (CacheLine *)malloc(slices_*lines_*sizeof(CacheLine));
  c_memory_ = new CacheMemory(config, subsets, memory_);

  policy_ = policy.getCopy();
  policy_->setSubsets(subsets);
  policy_->setMemory(memory_);
  policy_->setCMemory(c_memory_);

  R_ = R.getCopy();

  clearCache();

  srand(time(0));

/*
  printf("\n-------------------------------------\n");
  printf("GENERIC CACHE PARAMETERS:\n");
  printf("size: %2.2fMB, lines: %u, subsets: %u, sets: %u, ways: %u, slices: %u, linesize: %lu\n", (float)size_ / 1024 / 1024, lines_, subsets_, sets_, ways_, slices_, line_size_);
  printf("Eviction policy: %s\n", policy_name_.c_str());
  printf("R function: %s\n", R_->name_.c_str());
  printf("-------------------------------------\n\n");
*/

}

void GenericCache::clearCache()
{
  memset(memory_, 0, slices_*lines_*sizeof(CacheLine));
  memset(occupied_lines_, 0, sizeof(occupied_lines_));
  memset(cache_misses_, 0, sizeof(cache_misses_));
  memset(cache_hits_, 0, sizeof(cache_hits_));
  policy_->reset();
}

CLState GenericCache::isCached(size_t addr, size_t secret)
{
  CLState cl_state = MISS;
  unsigned slice = getSlice(addr);
  CacheSet *set = R_->getIndices(addr, secret);

  size_t aligned_addr = (addr >> line_size_bits_) << line_size_bits_;
  for (unsigned subset = 0; subset < subsets_; subset++)
  {
    for (uint32_t way = 0; way < subset_ways_; ++way)
    {
      CacheLine line = (*c_memory_)(slice, subset, set, way);
      if (aligned_addr == line.addr && line.valid == 1)
      {
        cl_state = HIT;
        break;
      }
    }
  }

  return cl_state;
}

AccessResult GenericCache::access(Access mem_access)
{
  /*printf("[");
  for (int i = 0; i < lines_; i++)
    printf((i == lines_ - 1 ? "%d]" : "%d, "), memory_[i].time);
  printf("\n");*/
  AccessResult cl_state = {MISS, false, 0};

  unsigned slice = getSlice(mem_access.addr);
  //assert(slice < slices_ && "Slice index too big");

  CacheSet *set =  R_->getIndices(mem_access.addr, mem_access.secret);

  size_t aligned_addr = (mem_access.addr >> line_size_bits_) << line_size_bits_;

  //look for address in set
  unsigned subset, way;
  for (subset = 0; subset < subsets_; subset++)
  {
    for (way = 0; way < subset_ways_; ++way)
    {
      CacheLine line = (*c_memory_)(slice, subset, set, way);
      if (aligned_addr == line.addr && line.valid == 1)
      {
        cl_state.hit = HIT;
        break;
      }
    }
    if (cl_state.hit)
      break;
  }

  if (cl_state.hit == HIT)
    cache_hits_[slice]++;
  else
    cache_misses_[slice]++;

  CacheLine* line = NULL;
  //select victim way & insert/replace
  if (cl_state.hit != HIT)
  {
    if (!policy_->isUMO())
    {
      InsertionPosition ins_pos = policy_->insertionPosition(set, slice);
      // can do this as pointers aswell maybe?
      subset = ins_pos.subset;
      way = ins_pos.way;

      line = &(*c_memory_)(slice, subset, set, way);
    }
    else
    {
      InsertionPosition ins_pos = policy_->updateAndInsert(&line, set, slice);
      subset = ins_pos.subset;
      way = ins_pos.way;

      // line calculated in updateAndInsert
    }

    //return evicted address so we can do inclusive cache hierarchies
    if (line->valid && cl_state.hit == MISS)
    {
      cl_state.evicted = true;
      cl_state.evicted_addr = line->addr;
    }

    //replace victim line
    line->addr = aligned_addr;
  }
  else
  {
    line = &(*c_memory_)(slice, subset, set, way);
  }

  policy_->updateSet(*line, set, slice, subset, way, cl_state.hit == MISS);

  //set valid
  line->valid = true;

  //mark used
  if (!line->used)
  {
    occupied_lines_[slice]++;
    line->used = true;
  }

#if DEBUG_SET
    printf("set: ");
    for (unsigned i = 0; i < subsets_; i++)
      printf("%u, ", set.index[i]);
    printf("\n");
#endif

#if DEBUG
  switch (policy_)
  {
    case RP_BIP:
    case RP_LRU:
    {
      printf("addr: %8lx, aligned: %8lx, slice: %2u, subset: %2u, set: %5u, way: %2u, ages: ", mem_access.addr, aligned_addr, slice, subset, set.index[subset], way);
      for (unsigned i = 0; i < subsets_; i++)
        for (unsigned j = 0; j < subset_ways_; j++)
          printf("%d%c ", (*c_memory_)(slice, i, set, j).time, (*c_memory_)(slice, i, set, j).valid ? ' ' : 'i');
      printf("\n");
      break;
    }

    
    case RP_PLRU:
    {
      std::bitset<15> bits(plru_sets_[slice * subsets_ * sets_ + subset * sets_ + set.index[subset]]);
      printf("addr: %8lx, aligned: %8lx, slice: %2u, subset: %2u, set: %5u, way: %2u, mask: ", mem_access.addr, aligned_addr, slice, subset, set.index[subset], way);
      printf("%s\n", bits.to_string().c_str());
      break;
    }

    default:
      break;
  }
#endif

  return cl_state;
}

void GenericCache::resetUsage(int slice)
{
  for (uint32_t i = 0; i < slices_*lines_; i++)
    memory_[i].used = 0;
  for (unsigned i = 0; i < slices_; i++)
    occupied_lines_[i] = 0;
}

void GenericCache::flush(Access mem_access)
{
  unsigned slice = getSlice(mem_access.addr);
  CacheSet *set =  R_->getIndices(mem_access.addr, mem_access.secret);

  size_t aligned_addr = (mem_access.addr >> line_size_bits_) << line_size_bits_;

  //look for address in set
  unsigned subset, way;
  bool found = false;
  for (subset = 0; subset < subsets_; subset++)
  {
    for (way = 0; way < subset_ways_; ++way)
    {
      CacheLine line = (*c_memory_)(slice, subset, set, way);
      if (aligned_addr == line.addr && line.valid == 1)
      {
        line.valid = false;
        break;
      }
    }
    if (found)
      break;
  }
}

unsigned GenericCache::getSlice(size_t phys_addr)
{
  return R_->getSlice(phys_addr);
}

//compatibility function for sc tests
CacheSet *GenericCache::getScatterSet(uint64_t secret, size_t phys_addr)
{
  return R_->getIndices(phys_addr, secret);
}

void GenericCache::resetBookkeeping(int slice){
  cache_hits_[0] = 0;
  cache_misses_[0] = 0;
}


GenericCache::~GenericCache()
{
  //printf("standard cache destroyed\n");
  free(memory_);
  delete c_memory_;
  delete policy_;
  delete R_;
}

