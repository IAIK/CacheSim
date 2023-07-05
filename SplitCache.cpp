#include <cstdlib>
#include <stdio.h>
#include <string.h>

#include "SplitCache.h"


SplitCache::SplitCache(Cache *instr_cache, Cache *data_cache, bool debug)
{
  instr_cache_ = instr_cache;
  data_cache_ = data_cache;

  name_ = "SplitCache";


  clearCache();

  if (debug)
  {
    printf("\n-------------------------------------");
    printf("\nI, type: %s, RPolicy: %s\n", instr_cache_->getName().c_str(), instr_cache_->getPolicy().c_str());
    printf("size: %2.2fMB, lines: %u, ways: %u, slices: %u, linesize: %lu\n", (float)instr_cache_->getCacheSize()/1024/1024, instr_cache_->getLines(), instr_cache_->getWays(), instr_cache_->getSlices(), instr_cache_->getLineSize());
    printf("\nD, type: %s, RPolicy: %s\n", data_cache_->getName().c_str(), data_cache_->getPolicy().c_str());
    printf("size: %2.2fMB, lines: %u, ways: %u, slices: %u, linesize: %lu\n", (float)data_cache_->getCacheSize()/1024/1024, data_cache_->getLines(), data_cache_->getWays(), data_cache_->getSlices(), data_cache_->getLineSize());

    printf("-------------------------------------\n\n");
  }


}

void SplitCache::clearCache()
{
    instr_cache_->clearCache();
    data_cache_->clearCache();
}

CLState SplitCache::isCached(size_t addr, size_t secret)
{
  if (instr_cache_->isCached(addr, secret) || data_cache_->isCached(addr, secret))
    return HIT;
  
  return MISS;
}

AccessResult SplitCache::access(Access mem_access)
{
  AccessResult result = {MISS, false, 0};
  
  if (mem_access.instruction)
    result = instr_cache_->access(mem_access);
  else
    result = data_cache_->access(mem_access);
  
  if (result.hit)
    cache_hits_[0]++;
  else
    cache_misses_[0]++;
    
  return result;
}

void SplitCache::resetUsage(int slice)
{
  instr_cache_->resetUsage(slice);
  data_cache_->resetUsage(slice);
}

void SplitCache::flush(Access mem_access)
{
  instr_cache_->flush(mem_access);
  data_cache_->flush(mem_access);
}

SplitCache::~SplitCache()
{
}
