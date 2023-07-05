#ifndef CACHE_HIERARCHY_H
#define CACHE_HIERARCHY_H

#include "Cache.h"

class CacheHierarchy : public Cache
{
  public:
    CacheHierarchy(int levels, Cache **caches, bool inclusive, bool debug = false);
    void clearCache();
    CLState isCached(size_t addr, size_t secret);
    AccessResult access(Access mem_access);
    void resetUsage(int slice);
    void flush(Access mem_access);
    uint32_t getUsage(int slice)
    {
      return caches_[levels_ - 1]->getUsage(slice);
    };
    uint64_t hits(int slice)
    {
      return cache_hits_[0];
    };
    uint64_t misses(int slice)
    {
      return cache_misses_[0];
    };

    int checkInclusivity(int level, size_t secret);

    ~CacheHierarchy();

    uint64_t inclusivity_flushes_;
  
  private:
    bool inclusive_;
    int levels_;
    Cache **caches_;
};
#endif // CACHE_HIERARCHY_H