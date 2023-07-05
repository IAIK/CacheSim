#ifndef GENERIC_CACHE_H
#define GENERIC_CACHE_H

#include "Cache.h"
#include "RMapper.h"
#include "RPolicy.h"

class GenericCache : public Cache
{
  public:
    GenericCache(CacheConfig config, RPolicy &policy, RMapper &R, unsigned P);
    void clearCache();
    CLState isCached(size_t addr, size_t secret);
    AccessResult access(Access mem_access);
    void resetUsage(int slice);
    void resetBookkeeping(int slice);
    void flush(Access mem_access);
    unsigned getSlice(size_t phys_addr) override;
    CacheSet *getScatterSet(uint64_t secret, size_t phys_addr);
    unsigned getSubsets() { return subsets_; };

    ~GenericCache();
  
  private:
    unsigned sets_;
    unsigned subsets_;
    unsigned subset_ways_;
    unsigned subset_way_bits_;
    RPolicy *policy_;
    RMapper *R_;

};
#endif // GENERIC_CACHE_H