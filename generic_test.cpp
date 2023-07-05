#include "SACache.h"
#include "GenericCache.h"
#include "CacheMemory.h"
#include "time.h"

#include "RP_PLRU.h"

int main() 
{
  CacheConfig config = {4, 3, 4, 6};
  RP_PLRU policy(config);
  Cache *sa_cache = new SACache(config, policy);
  unsigned P = 1;
  RMapper R(config, P, 0);
  Cache *g_cache = new GenericCache(config, policy, R, P);
  

  size_t t = clock();
  for (int i = 0; i < 1024*1024*16; i++)
  {
    uint64_t addr = 64*(rand() % 2048);
    AccessResult r_a = sa_cache->access({addr, 0, false, false});
    AccessResult r_g = g_cache->access({addr, 0, false, false});

    if (r_a.hit != r_g.hit)
      printf("wrong miss\n");
    if (r_a.evicted && (r_a.evicted_addr != r_g.evicted_addr))
      printf("different evicted addr\n");
  }
  t = clock() - t;

  printf("Clock ticks passed: %ld\n", t);

  delete sa_cache;
  delete g_cache;

  return 0;
}