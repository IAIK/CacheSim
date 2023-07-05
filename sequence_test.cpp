#include <algorithm>
#include <fstream>
#include "SACache.h"
#include "GenericCache.h"
#include "RP_QLRU.h"
#include "RP_LRU.h"
#include "RP_BIP.h"
#include "RP_RANDOM.h"
#include "RP_PLRU.h"

#define MAX_ADDRESSES 10000000
#define MAX_ERROR 0.001
#define EXACT 0.03125
#define MAX_SEQUENCE_LEN 100

RPolicy* getRPolicyObject(string type, CacheConfig config);

int main(int argc, char* argv[]) {
  string policy_name = "";
  if (argc > 1)
    policy_name = string(argv[1]);

  CacheConfig config = {3, 3, 1, 0};
  RPolicy* policy = getRPolicyObject(policy_name, config);
  
  /* GENERIC CACHE CONFIG */
  unsigned subsets = 1;
  RMapper R(config, subsets, 0);

  printf("Testing with policy: %s\n", policy->getPolicyName().c_str());

  unsigned addresses[MAX_SEQUENCE_LEN];

  std::ifstream file("sequence_test_input.txt");
  std::string s;

  while(getline(file, s))
  {
    if (s.at(0) == '%')
      continue;

    size_t idx = 0;
    size_t pos = 0;
    std::string token;
    std::string delimiter = " ";

    // split input sequence
    while ((pos = s.find(delimiter)) != std::string::npos) {
      token = s.substr(0, pos);

      // erase ?
      token.erase(remove(token.begin(), token.end(), '?'), token.end());
      addresses[idx] = stoi(token);

      // remove element from s
      s.erase(0, pos + delimiter.length());
      idx++;
    }
    // set last element
    addresses[idx] = stoi(s);

    auto *cache = new SACache(config, *policy);
    /* GENERIC CACHE */
    //auto* cache = new GenericCache(config, *policy, R, subsets);

    for (size_t i = 0; i <= idx; i++)
    {
      cache->access({addresses[i], 0, false, false});
    }

    printf("%ld\n", cache->hits(0));
    delete cache;
  }

  delete policy;
  return 0;
}

RPolicy *getRPolicyObject(string type, CacheConfig config)
{
  if (type == "RP_RANDOM")
    return new RP_RANDOM(config);
  else if (type == "RP_LRU")
    return new RP_LRU(config);
  else if (type == "RP_PLRU")
    return new RP_PLRU(config);
  else if (type == "RP_BIP")
    return new RP_BIP(config);
  else if (type == "RP_QLRU")
    return new RP_QLRU(config, {H00, M0, R0, U0, false});
  else
    return new RP_LRU(config);
}