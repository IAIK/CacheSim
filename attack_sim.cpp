#include <memory>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <deque>

#include "ScatterCache.h"
#include "SACache.h"
#include "CacheHierarchy.h"
#include "SplitCache.h"
#include "GenericCache.h"
#include "RSHA256.h"

#define L3_MISS_AVERAGE 220
#define L3_HIT_AVERAGE 50

void evacuate_cache_probabilistic(Cache *cache, size_t addr_start, unsigned mem_mult, unsigned accesses)
{
  size_t mem_length = cache->getLineSize()*cache->getSlices()*cache->getLines()*mem_mult;

  for (unsigned k = 0; k <= accesses; k++)
  {
    for (size_t addr = addr_start; addr < addr_start + mem_length; addr += cache->getLineSize())
      cache->access({addr, 0, false, false});
  }
}

void fillCache(Cache *cache, uint64_t secret)
{
  int i = 0;
  while(cache->getUsage(0) != cache->getLines())
    cache->access({(i++)*cache->getLineSize(), secret, false, false});
  //printf("filled after %d accesses\n", i);
}

//construct an eviction set without knowledge of the cache key
size_t* find_eviction_set_random(Cache *cache, uint32_t set_size, size_t target_address, bool shared_mem)
{
  size_t *eviction_addresses = (size_t *)malloc(sizeof(size_t)*set_size);
  uint32_t addresses_found = 0;

  const unsigned clear_mem_mult = 2;
  const unsigned clear_accesses = 1;

  
  //start at some random point in ram below 2GB
  size_t start_address = rand() & ~(cache->getLineSize() - 1);
  size_t test_address_start = start_address + cache->getLineSize()*cache->getSlices()*cache->getLines()*clear_mem_mult;

  int check_runs = 100; //how often per test_address we try to evict the target, 44->50%, 102->80% detection chance
  if (!shared_mem)  //need a lot more without shared memory
    check_runs *= cache->getWays();

  const int addr_per_loop = 10000*set_size; //more addresses -> more memory & more fasterer

  int refinds = 0;

  unsigned target_address_accesses = 0;

  int blocks = 0;
  int last_block_runs = 0;

  while (addresses_found < set_size)
  {
    for (int i = 0; i < check_runs; i++)
    {
      last_block_runs = i;
      for (size_t test_address = test_address_start; test_address < test_address_start + cache->getLineSize()*addr_per_loop; test_address += cache->getLineSize())
      {
        CLState access_result;
        if (shared_mem)
        {
          target_address_accesses += 2;
          cache->access({target_address, 0, false, false});
          cache->access({test_address, 0, false, false});
          access_result = cache->access({target_address, 0, false, false}).hit;
        }
        else
        {
          //try to clear target set with previously found addresses every few addresses
          //if (!(((test_address - test_address_start)/LINESIZE) % 100))
          //  for (int j = 0; j < addresses_found; j++)
          //    sc_access(eviction_addresses[j], 0, false, true);
          

          target_address_accesses++;
          cache->access({test_address, 0, false, false});
          cache->access({target_address, 0, false, false});
          access_result = cache->access({test_address, 0, false, false}).hit;
        }
        if (access_result == MISS)
        {
          bool unique = true;
          //check if we already found this one
          for (unsigned j = 0; j < addresses_found; j++)
          {
            if(eviction_addresses[j] == test_address)
            {
              refinds++;
              unique = false;
              break;
            }
          }
          if (unique)
          {
            eviction_addresses[addresses_found++] = test_address;
            if (addresses_found == set_size)
              goto done;
          }
        }
      }

      printf("%d\r", i);
      fflush(stdout);
      //cache->clearCache();
      //fillCache(cache, 25473564);
      evacuate_cache_probabilistic(cache, start_address, clear_mem_mult, clear_accesses);
      
    }
    printf("block %d, ran %d times, found %d\n", blocks, last_block_runs, addresses_found);
    blocks++;
    test_address_start +=  + cache->getLineSize()*addr_per_loop;
  }

  done:
  printf("block %d, ran %d times, found %d\n\n", blocks, last_block_runs, addresses_found);
  printf("found %d addresses, total mem: %3.1fMB\n", set_size, (float)(test_address_start + cache->getLineSize()*addr_per_loop - start_address)/1024/1024);
  printf("%u target address accesses,  %d double detections\n", target_address_accesses, refinds);
  
  

  return eviction_addresses;
}

uint32_t prime_and_probe(Cache *cache, size_t *eviction_set, uint32_t set_size, uint64_t secret)
{
  int misses = 0;
  for (unsigned i = 0; i < set_size; i++)
    if (cache->access({eviction_set[i], secret, false, false}).hit == MISS)
      misses++;
  return misses;
}

//int checkEvictionSet(GenericCache *sc, size_t* eviction_set, int address_count, size_t target_address, size_t target_secret, size_t attacker_secret)
int checkEvictionSet(ScatterCache *sc, size_t* eviction_set, int address_count, size_t target_address, size_t target_secret, size_t attacker_secret)
{
  size_t target_slice = sc->getSlice(target_address);
  CacheSet target_scatter_set = sc->getScatterSet(target_secret, target_address);

  int wrong_addresses = 0;

  for (int i = 0; i < address_count; i++)
  {
    CacheSet candidate_scatter_set;

    if (sc->getSlice(eviction_set[i]) != target_slice)
      continue;

    candidate_scatter_set = sc->getScatterSet(attacker_secret, eviction_set[i]);
    bool has_collision = false;
    for (unsigned j = 0; j < sc->getWays(); j++)
    {
      if (candidate_scatter_set.index[j] == target_scatter_set.index[j])
      {
        has_collision = true;
        break;
      }
    }
    if (!has_collision)
      wrong_addresses++;
  }

  return wrong_addresses;
}

//void eviction_set_gen_test(GenericCache *sc, size_t target_address, int address_count, bool shared_mem)
void eviction_set_gen_test(ScatterCache *sc, size_t target_address, int address_count, bool shared_mem)
{
  printf("\n-------------------------------------\n");
  printf("eviction set generation for %d addresses\n", address_count);
  printf("-------------------------------------\n\n");

  uint64_t hits = sc->hits(0);
  uint64_t misses = sc->misses(0);

  size_t *eviction_set = find_eviction_set_random(sc, address_count, target_address, shared_mem);
  int wrong_addresses = checkEvictionSet(sc, eviction_set, address_count, target_address, 0, 0);
  printf("%d wrong addresses\n", wrong_addresses);

  printf("hits: %lu, misses: %lu\n", (sc->hits(0) - hits)*sc->getSlices(), (sc->misses(0) - misses)*sc->getSlices());
  uint64_t hit_c = (sc->hits(0) - hits)*sc->getSlices()*L3_HIT_AVERAGE;
  uint64_t miss_c = (sc->misses(0) - misses)*sc->getSlices()*L3_MISS_AVERAGE;
  printf("est. %luc (%5.3fms @3GHz)\n", hit_c + miss_c, (float)(hit_c + miss_c)/3000000);

  free(eviction_set);
}


//generate a set with a specified number of addresses that have at least
//min_collisions with the target set in any place
//size_t* gen_eviction_set_random_ext(GenericCache *sc, int set_size, unsigned min_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, size_t start_addr)
size_t* gen_eviction_set_random_ext(ScatterCache *sc, int set_size, unsigned min_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, size_t start_addr)
{
  size_t *eviction_addresses = (size_t *) malloc(sizeof(size_t)*set_size);

  if (min_collisions > sc->getWays())
    exit(-1);

  uint32_t addr_collisions[MAX_WAYS] = {0};
  
  size_t target_slice = sc->getSlice(target_address);
  CacheSet target_scatter_set = sc->getScatterSet(target_secret, target_address);

  if (start_addr == 0)
    start_addr = (rand() & ~(sc->getLineSize() - 1)) - sc->getLineSize();
  size_t test_address = start_addr;

  for (int i = 0; i < set_size; i++)
  {
    unsigned collisions = 0;
    CacheSet candidate_scatter_set;
    while (collisions < min_collisions)
    {
      collisions = 0;
      if (!(test_address % 0x100000))
      {
        //printf(".");
      }
      test_address += sc->getLineSize();
      if (sc->getSlice(test_address) != target_slice)
        continue;
      candidate_scatter_set = sc->getScatterSet(attacker_secret, test_address);

      for (unsigned j = 0; j < sc->getWays(); j++)
      {
        if (candidate_scatter_set.index[j] == target_scatter_set.index[j])
            collisions++;
        /*/*simplified with changed way addressing
        for (int k = 0; k < sc_ways; k++)
        {
          if (candidate_scatter_set.index[j] == target_scatter_set.index[k])
          {
            collisions++;
            break;
          }
        }*/
      }
    }
    addr_collisions[collisions]++;
    /*printf("\nfound address 0x%lx with %d collisions\n", test_address, collisions);
    printf("set: ");
    for (int i = 0; i < sc_ways; i++)
      printf("%u, ", candidate_scatter_set.index[i]);
    printf("\n");*/

    eviction_addresses[i] = test_address;
  }

  printf("random address total mem: %3.1fMB\n", (float)(eviction_addresses[set_size-1] - start_addr)/1024/1024);

  /*printf("found %d addresses that have collisions with the target address:\n", set_size);
  for (int i = 1; i < sc_ways + 1; i++)
    printf("%d collisions: %4d\n", i, addr_collisions[i]);
  printf("\n");*/
  return eviction_addresses;
}

//generate a set with a [collisions]*WAYS addresses with exactly
//one collision per address, and [collisions] per line in the target set
//size_t* gen_eviction_set_per_line_ext(GenericCache *sc, unsigned target_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, bool allow_overlap, bool transpose, size_t start_addr)
size_t* gen_eviction_set_per_line_ext(ScatterCache *sc, unsigned target_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, bool allow_overlap, bool transpose, size_t start_addr)
{
  size_t *eviction_addresses = (size_t *) malloc(sizeof(size_t)*sc->getWays()*target_collisions);

  unsigned found_collisions[MAX_WAYS] = {0};
  CacheSet eviction_sets[MAX_WAYS*target_collisions];
    
  size_t target_slice = sc->getSlice(target_address);
  CacheSet target_scatter_set = sc->getScatterSet(target_secret, target_address);
  
  size_t test_address = -sc->getLineSize();
  if (start_addr != 0)
    test_address = start_addr - sc->getLineSize();

  unsigned min_collisions = 0;
  unsigned min_collisions_way = 0;

  while(min_collisions < target_collisions)
  {
    CacheSet candidate_scatter_set;
    unsigned collisions = 0;

    if (!(test_address % 0x100000))
    {
      //printf(".");
    }

    test_address += sc->getLineSize();
    if (sc->getSlice(test_address) != target_slice)
      continue;
    candidate_scatter_set = sc->getScatterSet(attacker_secret, test_address);
    unsigned way = 0;
    for (unsigned i = 0; i < sc->getWays(); i++)
    {
        if (candidate_scatter_set.index[i] == target_scatter_set.index[i])
        {
          collisions++;
          way = i;
          break;
        }
    }

    if (collisions == 1 && found_collisions[way] < target_collisions)
    {
      //check for overlap with other addresses' sets
      if (!allow_overlap)
      {
        bool overlap = false;
        for (unsigned i = 0; i < sc->getWays(); i++) //stored sets
        {
          for (unsigned j = 0; j < found_collisions[i]; j++) //stored sets
          {
            for (unsigned k = 0; k < sc->getWays(); k++) //compare set ways
            {
              if ((eviction_sets[target_collisions*i + j].index[k] == candidate_scatter_set.index[k])
                     && !(i == way && k == way))
              {
                overlap = true;
                break;
              }
              /*simplified with changed way addressing
              for (int l = 0; l < sc->getWays(); l++) //candidate set's ways
              {
                if ((eviction_sets[target_collisions*i + j].index[k] == candidate_scatter_set.index[l])
                     && !(way == i && candidate_scatter_set.index[l] == target_scatter_set.index[way]))
                {
                  overlap = 1;
                  break;
                }
              }*/
              if (overlap)
                break;
            }
            if (overlap)
              break;
          }
          if (overlap)
            break;
        }
        if (overlap)
          continue;
      }

      eviction_sets[target_collisions*way + found_collisions[way]] = candidate_scatter_set;
      if (!transpose)
        eviction_addresses[target_collisions*way + found_collisions[way]] = test_address;
      else
        eviction_addresses[sc->getWays()*found_collisions[way] + way] = test_address;
      found_collisions[way]++;

      //keep track of minimum collisions
      if (way == min_collisions_way)
      {
        min_collisions++;
        for (unsigned i = 0; i < sc->getWays(); i++)
        {
          if (found_collisions[i] < min_collisions)
          {
            min_collisions = found_collisions[i];
            min_collisions_way = i;
          }
        }
      }

      /*printf("\nfound address 0x%lx with 1 collision, min: %d\n", test_address, min_collisions);
      printf("set: ");
      for (int i = 0; i < sc_ways; i++)
        printf("%u, ", candidate_scatter_set.index[i]);
      printf("\n");*/
    }
  }
  return eviction_addresses;
}

//size_t* gen_eviction_set_per_line(GenericCache *sc, int target_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, bool allow_overlap, bool transpose)
size_t* gen_eviction_set_per_line(ScatterCache *sc, int target_collisions, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, bool allow_overlap, bool transpose)
{
  return gen_eviction_set_per_line_ext(sc, target_collisions, target_address, target_secret, attacker_secret, allow_overlap, transpose, 0);
}

//generate a set with [address_count] addresses that collide with the
//target address in at least one way
//size_t* find_eviction_set_purnal_pruning(GenericCache *sc, int address_count, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, size_t start_addr)
size_t* find_eviction_set_purnal_pruning(ScatterCache *sc, int address_count, size_t target_address, uint64_t target_secret, uint64_t attacker_secret, size_t start_addr = 0)
{

  size_t *eviction_addresses = (size_t *) malloc(sizeof(size_t)*address_count);
  
  size_t target_slice = sc->getSlice(target_address);
  CacheSet target_scatter_set = sc->getScatterSet(target_secret, target_address);

  if (start_addr == 0)
    start_addr = (rand() & ~(sc->getLineSize() - 1)) - sc->getLineSize();
  size_t test_address = start_addr;

  int collisions = 0;

  int k = sc->getLines() / 2;

  int collision_ways[MAX_WAYS] = {0};

  int victim_accesses = 0;

  while (collisions < address_count)
  {
    //create test address set
    std::deque<size_t> test_set(k);
    for (int i = 0; i < k; i++)
    {
      test_set[i] = test_address + sc->getLineSize() * (i + k*victim_accesses);
      sc->access({test_set[i], attacker_secret, 0, 0});
    }

    //settle set
    for (int i = 0; i < 5; i++)
      for (auto it = test_set.begin(); it != test_set.end(); it++)
        sc->access({*it, attacker_secret, 0, 0});

    //prune set
    bool pruned = false;
    unsigned prunings = 0;
    while (!pruned)
    {
      pruned = true;
      for (auto it = test_set.begin(); it != test_set.end();)
      {
        if (!sc->access({*it, attacker_secret, 0, 0}).hit)
        {
          pruned = false;
          it = test_set.erase(it);
          continue;
        }
        it++;
      }
      prunings++;
    }
    printf("prunings: %u, size: %lu, ", prunings, test_set.size());

    //access victim
    sc->access({target_address, target_secret, false, false});
    victim_accesses++;

    bool access_detected = false;
    //find evicted address
    for (auto it = test_set.begin(); it != test_set.end(); it++)
    {
      if (!sc->access({*it, attacker_secret, 0, 0}).hit)
      {
        eviction_addresses[collisions++] = *it;
        access_detected = true;
        test_set.erase(it);
        break;
      }
    }

    if (access_detected)
    {
      size_t candidate_slice = sc->getSlice(eviction_addresses[collisions-1]);
      CacheSet candidate_scatter_set = sc->getScatterSet(target_secret, eviction_addresses[collisions-1]);

      int cols = 0;
      if (candidate_slice == target_slice)
      {
        for (unsigned i = 0; i < sc->getWays(); i++)
        {
          if (candidate_scatter_set.index[i] == target_scatter_set.index[i])
          {
            cols++;
            collision_ways[i]++;
          }
        }
      }
      printf("addr: %lx, collisons: %d", eviction_addresses[collisions-1], cols);
    }
    printf("\n");

    // clear cache
    //sc->clearCache();
    //fillCache(sc, 25473564);
  }

  printf("way distribution: ");
  for (unsigned i = 0; i < sc->getWays(); i++)
    printf("%d ", collision_ways[i]);
  printf("\n");

  printf("%d victim accesses\n", victim_accesses);
  
  printf("random address total mem: %3.1fMB\n", (float)(sc->getLineSize() * (k + k*victim_accesses))/1024/1024);

  return eviction_addresses;
}

//void eviction_set_gen_pruning_test(GenericCache *sc, int address_count, size_t target_address, uint64_t target_secret, uint64_t attacker_secret)
void eviction_set_gen_pruning_test(ScatterCache *sc, int address_count, size_t target_address, uint64_t target_secret, uint64_t attacker_secret)
{
  printf("\n-------------------------------------\n");
  printf("eviction set generation for %d addresses w/ Purnal pruning\n", address_count);
  printf("-------------------------------------\n\n");

  uint64_t hits = sc->hits(0);
  uint64_t misses = sc->misses(0);

  size_t *eviction_set = find_eviction_set_purnal_pruning(sc, 275, target_address, target_secret, attacker_secret);

  int wrong_addresses = checkEvictionSet(sc, eviction_set, address_count, target_address, 0, 0);
  printf("%d wrong addresses\n", wrong_addresses);

  printf("hits: %lu, misses: %lu\n", (sc->hits(0) - hits)*sc->getSlices(), (sc->misses(0) - misses)*sc->getSlices());
  uint64_t hit_c = (sc->hits(0) - hits)*sc->getSlices()*L3_HIT_AVERAGE;
  uint64_t miss_c = (sc->misses(0) - misses)*sc->getSlices()*L3_MISS_AVERAGE;
  printf("est. %luc (%5.3fms @3GHz)\n", hit_c + miss_c, (float)(hit_c + miss_c)/3000000);

  free(eviction_set);
}


//prime and probe with one-shot eviction sets between prime/probe
//void p_p_attack_evict(GenericCache *sc, size_t target_address, uint64_t  target_secret, uint64_t  attacker_secret)
void p_p_attack_evict(ScatterCache *sc, size_t target_address, uint64_t  target_secret, uint64_t  attacker_secret)
{

  printf("\n-------------------------------------\n");
  printf("Prime+Probe one-shot eviction attack with confusion matrix\n");
  printf("-------------------------------------\n\n");

  const int target_addr_count = 8;
  const int detection_runs = 35; //17 -> 90% detection (8 way)
  const int eviction_addr_count = 275;

  size_t **eviction_sets = (size_t **) malloc(sizeof(size_t *)*target_addr_count);
  size_t **p_p_sets = (size_t **) malloc(sizeof(size_t *)*target_addr_count);

  for (int i = 0; i < target_addr_count; i ++)
  {
    eviction_sets[i] = gen_eviction_set_random_ext(sc, eviction_addr_count*detection_runs, 1, target_address + 64*i, target_secret, attacker_secret, 0);
    p_p_sets[i] = gen_eviction_set_per_line_ext(sc, detection_runs, target_address + 64*i, target_secret, attacker_secret, false, true, 2*1024*1024*1024LL);
  }
  

  uint64_t hits = sc->hits(0);
  uint64_t misses = sc->misses(0);
  
  //create confusion matrix
  int detections[32][32] = {{0}};
  for (int i = 0; i < 100; i++) //iterations for statistics
  {
    for (int l = 0; l < target_addr_count; l++) //on accessed target address per round
    {
      size_t current_address = target_address + l*64;
      for (int j = 0; j < target_addr_count; j++) //try each address each time
      {
        bool detected = false;
        for (int k = 0; k < detection_runs; k++) //detection attempts per address
        {
          prime_and_probe(sc, p_p_sets[j] + sc->getWays()*k, sc->getWays(), attacker_secret);
          sc->access({current_address, target_secret, false, false});
          uint32_t misses = prime_and_probe(sc, p_p_sets[j] + sc->getWays()*k, sc->getWays(), attacker_secret);
          prime_and_probe(sc, eviction_sets[j] + eviction_addr_count*k, eviction_addr_count, attacker_secret);
          if (misses)
          {
            detected = true;
            break;
          }
        }
        if (detected)
          detections[l][j]++;
      }
      sc->clearCache();
      fillCache(sc, 25473564);
      //evacuate_cache_probabilistic(sc, 4*1024*1024LL, 2, 1);
    }
  }

  uint64_t hit_c = (sc->hits(0) - hits)*sc->getSlices()*L3_HIT_AVERAGE;
  uint64_t miss_c = (sc->misses(0) - misses)*sc->getSlices()*L3_MISS_AVERAGE;
  printf("%lu hits, %lu misses\n", (sc->hits(0) - hits), (sc->misses(0) - misses));
  printf("est. %luc (%5.3fms @3GHz)\n", hit_c + miss_c, (float)(hit_c + miss_c)/3000000);

  for (int i = 0; i < target_addr_count; i++)
  {
    printf("%2d: ", i);
    for (int j = 0; j < target_addr_count; j++)
    {
      printf("%4d ", detections[i][j]);
    }
    printf("\n");
  }

  for (int i = 0; i < target_addr_count; i ++)
  {
    free(eviction_sets[i]);
    free(p_p_sets[i]);
  }
  free(eviction_sets);
  free(p_p_sets);

}


size_t target_address = 0x2680204054068000;
uint64_t target_secret = 0x54e6a4cf2f45ab5e;
uint64_t attacker_secret = 0x2a5b7ef9c3c49ae8;
//uint64_t target_secret = 0;
//uint64_t attacker_secret = 0;


int main(int argc, char** argv)
{ 

  unsigned cache_way_bits;
  unsigned cache_slices;
  unsigned cache_line_bits;

  float noise;

  unsigned test_nr = 0;

  if (argc > 1)
  {
    if (!sscanf(argv[1], "%u", &test_nr))
      exit(!fprintf(stderr,"expected the number of the test to be run\n"));
  }
  else
  {
    cache_line_bits = 13;
  }

  if (argc > 2)
  {
    if (!sscanf(argv[2], "%u", &cache_line_bits))
      exit(!fprintf(stderr,"expected the number of cache lines per slice in bits\n"));
  }
  else
  {
    cache_line_bits = 13;
  }
  
  if (argc > 3)
  {
    if (!sscanf(argv[3], "%u", &cache_way_bits))
      exit(!fprintf(stderr,"expected the number of cache ways in bits\n"));
  }
  else
  {
    cache_way_bits = 3;
  }

  if (argc > 4)
  {
    if (!sscanf(argv[4], "%u", &cache_slices))
      exit(!fprintf(stderr,"expected the number of slices\n"));
  }
  else
  {
    cache_slices = 1;
  }

  if (argc > 5)
  {
    if (!sscanf(argv[5], "%f", &noise))
      exit(!fprintf(stderr,"expected a float for noise, [0-1[\n"));
  }
  else
  {
    noise = 0;
  }

  ScatterCache *sc;

  if (noise != 0)
    sc = new ScatterCache({cache_line_bits, cache_way_bits, cache_slices, 6}, SC_V1, noise);
  else
    sc = new ScatterCache({cache_line_bits, cache_way_bits, cache_slices, 6}, SC_V1);

  //RSHA256 R({cache_line_bits, cache_way_bits, cache_slices, 6}, 1<<cache_way_bits, 0);
  //GenericCache *sc = new GenericCache({cache_line_bits, cache_way_bits, cache_slices, 6}, RP_LRU, R, 1<<cache_way_bits);
    

  fillCache(sc, 25473564);

  switch (test_nr)
  {
    case 0: {
      eviction_set_gen_test(sc, target_address, 275, false);
      break;
    }
    case 1: {
      p_p_attack_evict(sc, target_address, target_secret, attacker_secret);
      break;
    }
    case 2: {
      eviction_set_gen_pruning_test(sc, 275, target_address, target_secret, attacker_secret);
      break;
    }
    default:
      exit(!fprintf(stderr,"no test with number %d\n", test_nr));
  }

  //target_address_eviction_test(sc, target_address, target_secret, attacker_secret);
  

  return 0;
}