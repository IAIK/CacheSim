#include <memory>
#include <stdio.h>
#include <fstream>
#include <string.h>

#include "ScatterCache.h"
#include "SACache.h"
#include "CacheHierarchy.h"
#include "SplitCache.h"

void fillCache(Cache *cache, uint64_t secret)
{
  int i = 0;
  while(cache->getUsage(0) != cache->getLines())
    cache->access({(i++)*cache->getLineSize(), secret, false, false});
  //printf("filled after %d accesses\n", i);
}

#define N_LOOPS 100

int main(int argc, char** argv)
{ 
  float eviction_rate[6];
  uint64_t fill_secret = 0x54e6a4cf2f45ab5e;

  for (unsigned way_bits = 0; way_bits < 6; way_bits++)
  {
    ScatterCache *sc = new ScatterCache({13, way_bits, 1, 6}, SC_V1);
    uint64_t misses = 0;
    for (int i = 0; i < N_LOOPS; i++)
    {
      sc->clearCache();
      fillCache(sc, fill_secret);
      for (unsigned j = 0; j < sc->getLines()/sc->getWays(); j++)
        sc->access({j*sc->getLineSize(), 0, false, false});
      for (unsigned j = 0; j < sc->getLines()/sc->getWays(); j++)
        misses += sc->isCached(j*sc->getLineSize(), 0) == MISS;
    }
    eviction_rate[way_bits] = (float)misses/(N_LOOPS*sc->getLines()/sc->getWays());
    printf("%2d: %6.4f, %lu\n", 1 << way_bits, eviction_rate[way_bits], misses);
    delete sc;
  }

  return 0;
}