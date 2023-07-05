#ifndef SPLIT_CACHE_H
#define SPLIT_CACHE_H

#include "Cache.h"

class SplitCache : public Cache
{
  public:
    SplitCache(Cache *instr_cache, Cache *data_cache, bool debug = false);
    void clearCache();
    CLState isCached(size_t addr, size_t secret);
    AccessResult access(Access mem_access);
    void resetUsage(int slice);
    void flush(Access mem_access);
    uint32_t getUsage(int slice)
    {
      return data_cache_->getUsage(slice);
    };
    uint64_t hits(int slice)
    {
      return cache_hits_[0];
    };
    uint64_t misses(int slice)
    {
      return cache_misses_[0];
    };


    ~SplitCache();

    Cache *instr_cache_;
    Cache *data_cache_; 
  
  private:

};
#endif // SPLIT_CACHE_H