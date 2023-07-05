#ifndef STANDARD_CACHE_H
#define STANDARD_CACHE_H

#include "Cache.h"
#include "RPolicy.h"

class SACache : public Cache
{
  public:
    SACache(CacheConfig config, RPolicy &policy);
    void clearCache();
    CLState isCached(size_t addr, size_t secret);
    AccessResult access(Access mem_access);
    void resetUsage(int slice);
    void flush(Access mem_access);

    ~SACache();
  
  private:
    unsigned getSetIndex(size_t addr)
    {
      return (addr & ((sets_ - 1 ) << line_size_bits_)) >> line_size_bits_;
    }

    unsigned sets_;
    RPolicy *policy_;
};
#endif // STANDARD_CACHE_H