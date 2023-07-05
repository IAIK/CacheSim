#include "SACache.h"
#include "GenericCache.h"
#include "CacheMemory.h"
#include "time.h"
#include "RP_BIP.h"
#include "RP_RANDOM.h"
#include "RP_LRU.h"
#include "RP_PLRU.h"
#include "RP_QLRU.h"

#include "assert.h"

#define NUM_POLICIES 5
#define MAX_ITERATIONS 8

RPolicy* getPolicyObject(CacheConfig config, int policy_num);

int main()
{
  CacheConfig config = {4, 3, 4, 6};
  unsigned P = 1;
  RMapper R(config, P, 0);

  for (int i = 0; i < NUM_POLICIES; i++)
  {
    RPolicy* policy = getPolicyObject(config, i);

    uint64_t total_time = 0;

    for (int j = 0; j < MAX_ITERATIONS; j++)
    {
      auto* cache = new GenericCache(config, *policy, R, P);

      size_t t = clock();
      for (int i = 0; i < 1024 * 1024 * 16; i++)
      {
        uint64_t addr = 64 * (rand() % 2048);
        cache->access({ addr, 0, false, false });
      }
      t = clock() - t;

      total_time += t;

      delete cache;
    }

    printf("Policy: %s, Average time (16M accesses): %f\n", policy->getPolicyName().c_str(), (double) total_time / MAX_ITERATIONS);

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
  case 4:
    return new RP_QLRU(config, { H00, M0, R0, U0, false });
  default:
    assert(false);
    return NULL;
  }
}
