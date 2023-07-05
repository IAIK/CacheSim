#include <cstdlib>
#include <stdio.h>
#include <string.h>

#include "CacheHierarchy.h"


CacheHierarchy::CacheHierarchy(int levels, Cache **caches, bool inclusive, bool debug)
{
  inclusive_ = inclusive;
  inclusivity_flushes_ = 0;
  levels_ = levels;
  caches_ = (Cache **)malloc(levels*sizeof(Cache *));
  for (int level = 0; level < levels_; level++)
    caches_[level] = caches[level];

  clearCache();

  if (debug)
  {
    printf("\n-------------------------------------");
    for (int level = 0; level < levels_; level++)
    {
      printf("\nL%1d, type: %s, RPolicy: %s\n", level+1, caches_[level]->getName().c_str(), caches_[level]->getPolicy().c_str());
      printf("size: %2.2fMB, lines: %u, ways: %u, slices: %u, linesize: %lu\n", (float)caches_[level]->getCacheSize()/1024/1024, caches_[level]->getLines(), caches_[level]->getWays(), caches_[level]->getSlices(), caches_[level]->getLineSize());
    }
    printf("-------------------------------------\n\n");
  }



}

void CacheHierarchy::clearCache()
{
  for (int level = 0; level < levels_; level++)
    caches_[level]->clearCache();
}

CLState CacheHierarchy::isCached(size_t addr, size_t secret)
{
  for (int level = 0; level < levels_; level++)
    if (caches_[level]->isCached(addr, secret))
      return HIT;
  
  return MISS;
}

//not really tested for non-inclusive caches, use with care
AccessResult CacheHierarchy::access(Access mem_access)
{
  AccessResult result = {MISS, false, 0};
  AccessResult level_results[10];

  //go up the hierarchy looking for hits..
  int level;
  for (level = 0; level < levels_; level++)
  {
    level_results[level] = caches_[level]->access(mem_access);
    if (level_results[level].hit)
    {
      result.hit = HIT;
      if (inclusive_)
      {
        //in case of a write, notify inclusive upper caches regardless
        if (!mem_access.write)
          break;
      }
      else
      {
        //keep non-inclusive caches coherent //TODO is the last part a problem for 3-level non-inclusive caches where it's stored in level 1 and 3?
        if (!mem_access.write || (level == levels_ - 1) || !caches_[level + 1]->isCached(mem_access.addr, mem_access.secret))
          break;
      }
    }
    else
      result.evicted = true;
  }

  //.. and back down to flush evicted lines from lower level caches in inclusive structures
  if (inclusive_)
  {
    for (level = level == levels_ ? levels_ - 1 : level; level > 0; level--)
      if (level_results[level].evicted)
      {
        inclusivity_flushes_++;
        for (volatile int i = level; i > 0; i--)
        {
          caches_[i - 1]->flush({level_results[level].evicted_addr, mem_access.secret, false, false});
          //printf("L%1d: flushing %p\n", i-1, level_results[level].evicted_addr);
        }
      }
  }

  if (result.hit)
    cache_hits_[0]++;
  else
    cache_misses_[0]++;

  return result;
}

void CacheHierarchy::resetUsage(int slice)
{
  for (int level = 0; level < levels_; level++)
    caches_[level]->resetUsage(slice);
}

void CacheHierarchy::flush(Access mem_access)
{
  for (int level = 0; level < levels_; level++)
  {
    caches_[level]->flush(mem_access);
  }
}

int CacheHierarchy::checkInclusivity(int level, size_t secret)
{
  int missing = 0;
  
  for (unsigned i = 0; i < caches_[level - 1]->getLines()*caches_[level - 1]->getSlices(); i++)
    if (caches_[level - 1]->memory_[i].valid && !caches_[level]->isCached(caches_[level - 1]->memory_[i].addr, secret))
      missing++;

  return missing;
}

CacheHierarchy::~CacheHierarchy()
{
  free(caches_);
}
