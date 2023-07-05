#include <stdlib.h>

#include "SACache.h"
#include "GenericCache.h"
#include "CacheMemory.h"
#include "RP_LRU.h"

int main() 
{ 
  unsigned P = 1;

  srand(time(0));

  //test hits for exact set fit
  for (unsigned way_bits = 1; way_bits < 6; way_bits++)
  {
    unsigned ways = 1 << way_bits;
    CacheConfig config = {way_bits, way_bits, 1, 0};
    RP_LRU policy(config);
    RMapper R(config, P, 0);
    Cache *cache = new GenericCache(config, policy, R, P);

    //preload set
    for (unsigned i = 0; i < ways; i++)
    {
      size_t addr = i;
      cache->access({addr, 0, false, false});
    }
    //test it (not expecting a surprise here)
    for (unsigned i = 0; i < 5; i++)
    {
      for (unsigned j = 0; j < ways; j++)
      {
        size_t addr = j;
        if (!cache->access({addr, 0, false, false}).hit)
        {
          printf("unexpected miss, ways: %u, addr: %lx\n", ways, addr);
          return -1;
        }
      }
    }

    delete cache;
  }

  //test missing for overfull set
  for (unsigned way_bits = 0; way_bits < 6; way_bits++)
  {
    unsigned ways = 1 << way_bits;
    CacheConfig config = {way_bits, way_bits, 1, 0};
    RP_LRU policy(config);
    RMapper R(config, P, 0);
    Cache *cache = new GenericCache(config, policy, R, P);

    for (unsigned i = 0; i < 100; i++)
    {
      for (unsigned j = 0; j < ways+1; j++)
      {
        size_t addr = j;
        if (cache->access({addr, 0, false, false}).hit)
        {
          printf("unexpected hit, ways: %u, addr: %lx\n", ways, addr);
          return -1;
        }
      }
    }

    delete cache;
  }

  return 0;
}