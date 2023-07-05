#include "SACache.h"
#include "ScatterCache.h"
#include "CacheHierarchy.h"

#include "RP_PLRU.h"
#include "RP_LRU.h"

int main() 
{ 
  CacheConfig l1_config = {9, 3, 1, 6};
  CacheConfig l2_config = {12, 2, 1, 6};
  RP_PLRU l1_policy(l1_config);
  RP_PLRU l2_policy(l2_config);
  Cache *caches[2];
  caches[0] = new SACache(l1_config, l1_policy);
  caches[1] = new SACache(l2_config, l2_policy);

  Cache *cache = new CacheHierarchy(2, caches, true, true);

  cache->access({0});

  printf("cached in L1: %u\n", caches[0]->isCached(0,0));

  for (uint i = 1; i < caches[1]->getWays()+1; i++)
  {
    cache->access({i*caches[1]->getCacheSize()}); //fill L2 set 0 that contains the address above, evict from L1 even though L1 set 0 is not full
  }

  printf("cached in L1: %u\n", caches[0]->isCached(0,0));
  printf("flushes because of inclusivity: %lu\n", ((CacheHierarchy*)cache)->inclusivity_flushes_);

  delete cache;
  return 0;
}