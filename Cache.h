#ifndef CACHE_H
#define CACHE_H

#include <cstddef>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <pthread.h>

//#include <memory>


//using std::unique_ptr;
using std::string;

#define MAX_WAYS 32
#define MAX_SLICES 32
#define SLICE0 0x1B5F575440ULL
#define SLICE1 0x2EB5FAA880ULL
#define SLICE2 0x3CCCC93100ULL

class CacheMemory;

typedef enum { MISS = false, HIT = true } CLState;

struct Access {
  uint64_t addr;
  uint64_t secret = 0;
  bool write = false;
  bool instruction = false;
  uint8_t thread = 0;
};

struct CacheLine {
  uint64_t addr;
  int time;
  bool valid;
  bool used;
};

struct CacheConfig {
  unsigned line_bits;
  unsigned way_bits;
  unsigned slices;
  unsigned line_size_bits;
};

struct AccessResult {
  CLState hit;
  bool evicted;
  uint64_t evicted_addr;
};

struct CacheSet {
  uint32_t index[MAX_WAYS];
};

class Cache
{
  public:
    virtual void clearCache() = 0;
    virtual CLState isCached(size_t addr, size_t secret) = 0;
    virtual AccessResult access(Access mem_access) = 0;
    virtual void resetUsage(int slice) = 0;
    virtual void flush(Access mem_access) = 0;
    virtual uint32_t getUsage(int slice)
    {
      return occupied_lines_[slice];
    };
    virtual uint64_t hits(int slice)
    {
      return cache_hits_[slice];
    };
    virtual uint64_t misses(int slice)
    {
      return cache_misses_[slice];
    };
    virtual uint64_t accesses(int slice)
    {
      return cache_misses_[slice] + cache_hits_[slice];
    };
    virtual unsigned getSlice(size_t phys_addr)
    {
      int slice = 0;
      if (slices_ >= 2)
      {
        slice = __builtin_popcountll(phys_addr & SLICE0) % 2;
        if (slices_ >= 4)
        {
          slice |= (__builtin_popcountll(phys_addr & SLICE1) % 2) << 1;
          if (slices_ >= 8)
          {
            slice |= (__builtin_popcountll(phys_addr & SLICE2) % 2) << 2;
          }
        }
      }
      return slice;
    };
    virtual unsigned getCacheSize()
    {
      return size_;
    };
    virtual uint64_t getLineSize()
    {
      return line_size_;
    };
    virtual unsigned getLines()
    {
      return lines_;
    };
    virtual unsigned getWays()
    {
      return ways_;
    };
    virtual string getName()
    {
      return name_;
    };
    virtual string getPolicy()
    {
      return policy_name_;
    };
    virtual unsigned getSlices()
    {
      return slices_;
    };

    virtual ~Cache() {};

    CacheLine *memory_;
    CacheMemory *c_memory_;
   

  protected:

    string name_;
    string policy_name_;
    uint64_t line_size_;
    unsigned line_size_bits_;
    uint32_t lines_;
    unsigned line_bits_;
    unsigned ways_;
    unsigned way_bits_;
    unsigned slices_;
    uint32_t size_;
    uint32_t occupied_lines_[MAX_SLICES];
    uint64_t cache_misses_[MAX_SLICES];
    uint64_t cache_hits_[MAX_SLICES];

};


#endif // CACHE_H