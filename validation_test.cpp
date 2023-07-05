#include "SACache.h"
#include "GenericCache.h"
#include "CacheMemory.h"
#include "time.h"
#include "RP_BIP.h"
#include "RP_RANDOM.h"
#include "RP_LRU.h"
#include "RP_PLRU.h"
#include "RP_QLRU.h"

#include "RAES_NI.h"
#include "RCAT.h"
#include "RSASS_AES_NI.h"
#include "RSHA256.h"

#include "assert.h"

#define NUM_POLICIES 4
#define NUM_MAPPERS 5

RPolicy* getPolicyObject(CacheConfig config, int policy_num);
RMapper* getMapperObject(CacheConfig config, unsigned subsets, int mapper_num);

int main()
{
  srand(27);

  CacheConfig config = { 7, 4, 2, 6 };
  unsigned P = 2;
  
  for (int i = 0; i < NUM_POLICIES; i++)
  {
    RPolicy* policy = getPolicyObject(config, i);
    
    for (int j = 0; j < NUM_MAPPERS; j++)
    {
      RMapper* mapper = getMapperObject(config, P, j);
      auto* cache = new GenericCache(config, *policy, *mapper, P);

      for (int i = 0; i < 1024 * 1024 * 16; i++)
      {
        uint64_t addr = 64 * (rand() % 1024);
        //printf("%ld\n", addr);
        cache->access({ addr, 0, false, false });
      }

      long hits = 0;
      long misses = 0;
      for (unsigned i = 0; i < config.slices; i++)
      {
        hits += cache->hits(i);
        misses += cache->misses(i);
      }
      printf("Policy: %8s, Mapper: %12s, Config: %4d, %4d, %4d, %4d, Hits: %8ld, Misses: %8ld, Hitrate: %5f\n",
        policy->getPolicyName().c_str(), mapper->getMapperName().c_str(), config.line_bits, config.way_bits, config.slices, 
        config.line_size_bits, hits, misses, (float)hits / (hits + misses));

      delete mapper;
      delete cache;
    }
    printf("\n");
    delete policy;
  }


  return 0;
}

RPolicy* getPolicyObject(CacheConfig config, int policy_num)
{
  switch (policy_num)
  {
  case 0:
    return new RP_LRU(config);
  case 1:
    return new RP_PLRU(config);
  case 2:
    return new RP_BIP(config);
  case 3:
    return new RP_RANDOM(config);
  default:
    assert(false);
    return NULL;
  }
}

RMapper* getMapperObject(CacheConfig config, unsigned subsets, int mapper_num)
{
  switch (mapper_num)
  {
  case 0:
    return new RMapper(config, subsets, 0);
  case 1:
    return new RAES_NI(config, subsets, 0);
  case 2:
    return new RCAT(config, subsets, 1);
  case 3:
    return new RSASS_AES_NI(config, subsets, 0);
  case 4:
    return new RSHA256(config, subsets, 0);
  default:
    assert(false);
    return NULL;
  }
}

