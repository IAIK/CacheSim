#include <stdlib.h>

#include "SACache.h"
#include "GenericCache.h"
#include "CacheMemory.h"

#include "RP_PLRU.h"

int main() 
{ 
  unsigned P = 1;

  srand(time(0));

  //test hits for exact set fit
  for (unsigned way_bits = 1; way_bits < 6; way_bits++)
  {
    unsigned ways = 1 << way_bits;
    CacheConfig config = {way_bits, way_bits, 1, 0};
    RP_PLRU policy(config);
    RMapper R(config, P);
    GenericCache cache(config, policy, R, P);

    //preload set
    for (unsigned i = 0; i < ways; i++)
    {
      size_t addr = i;
      cache.access({addr, 0, false, false});
    }
    //test it (not expecting a surprise here)
    for (unsigned i = 0; i < 5; i++)
    {
      for (unsigned j = 0; j < ways; j++)
      {
        size_t addr = j;
        if (!cache.access({addr, 0, false, false}).hit)
        {
          printf("unexpected miss, ways: %u, addr: %lx, loop: %u\n", ways, addr, i);
          return -1;
        }
      }
    }
  }

  //test missing for overfull set
  for (unsigned way_bits = 1; way_bits < 6; way_bits++)
  {
    unsigned ways = 1 << way_bits;
    CacheConfig config = {way_bits, way_bits, 1, 0};
    RP_PLRU policy(config);
    RMapper R(config, P);
    GenericCache cache(config, policy, R, P);
    //SACache cache(config, RP_PLRU);

    //preload set first, because empty init order != plru order
    for (unsigned i = 0; i < ways; i++)
    {
      size_t addr = i+ways;
      cache.access({addr, 0, false, false});
    }

    for (unsigned i = 0; i < 100; i++)
    {
      for (unsigned j = 0; j < ways+1; j++)
      {
        size_t addr = j;
        if (cache.access({addr, 0, false, false}).hit)
        {
          printf("unexpected hit, ways: %u, addr: %lx, loop: %u\n", ways, addr, i);
          return -1;
        }
      }
    }
  }

  //test early eviction of non-oldest address
  for (unsigned way_bits = 2; way_bits < 6; way_bits++)
  {
    unsigned ways = 1 << way_bits;
    CacheConfig config = {way_bits, way_bits, 1, 0};
    RP_PLRU policy(config);
    RMapper R(config, P);
    //GenericCache cache(config, policy, R, P);
    SACache cache(config, policy);

    //preload set first, because empty init order != plru order
    for (unsigned i = 0; i < ways; i++)
    {
      size_t addr = i;
      cache.access({addr, 0, false, false});
    }

    for (unsigned i = 0; i < 100; i++)
    {
      size_t addr_start = rand();
      //define set order
      for (unsigned j = 1; j < ways+1; j++)
      {
        size_t addr = addr_start+j;
        cache.access({addr, 0, false, false});
      }

      //prime set to evict non-oldest
      for (unsigned j = ways/2; j >0; j/=2)
      {
        size_t addr = addr_start+j;
        cache.access({addr, 0, false, false});
      }

      //replace non-oldest
      cache.access({addr_start-1, 0, false, false});
      if (cache.access({addr_start+ways, 0, false, false}).hit)
      {
        printf("unexpected hit on non-oldest, ways: %u, addr: %lx, loop: %u\n", ways, addr_start+ways, i);
        return -1;
      }

    }
  }

  return 0;
}