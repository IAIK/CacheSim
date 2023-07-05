#ifndef CACHE_MEMORY_H
#define CACHE_MEMORY_H

#include <cstdint>

struct CacheConfig;
struct CacheLine;
struct CacheSet;

class CacheMemory
{
  public:
    CacheMemory(CacheConfig config, unsigned subsets);
    CacheMemory(CacheConfig config, unsigned subsets, CacheLine *memory);
    CacheLine& operator()(unsigned slice, unsigned subset, unsigned set, unsigned way);
    CacheLine& operator()(unsigned slice, unsigned subset, CacheSet *set, unsigned way);
    CacheLine& operator()(unsigned slice, unsigned set, unsigned way);

    ~CacheMemory();

  private:
    CacheLine *memory_;
    uint32_t lines_;
    unsigned ways_;
    unsigned slices_;
    unsigned sets_;
    unsigned subsets_;
    unsigned subset_ways_;
    bool external_memory_;
};
#endif // CACHE_MEMORY_H