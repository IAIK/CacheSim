#ifndef SCATTER_CACHE_H
#define SCATTER_CACHE_H

#include "Cache.h"

typedef enum {SC_V1, SC_V2} SCVersion;

class ScatterCache : public Cache
{
  public:
    ScatterCache(CacheConfig config, SCVersion version);
    ScatterCache(CacheConfig config, SCVersion version, float noise);
    void clearCache();
    CLState isCached(size_t addr, size_t secret);
    AccessResult access(Access mem_access);
    void resetUsage(int slice);
    void flush(Access mem_access);

    CacheSet getScatterSet(uint64_t secret, size_t phys_addr);
    AccessResult extAccess(size_t addr, bool write, size_t secret, bool quiet, size_t *test_set, uint32_t test_set_size, bool *test_hit, bool no_noise = false);

    ~ScatterCache();

    // could be void, only here because scv2_test needs this
    CacheSet getScatterSetV2(uint64_t secret, size_t phys_addr);
    CacheSet getScatterSetV1(uint64_t secret, size_t phys_addr);
  
  private:
    void randomAccess();
    SCVersion version_;
    bool noisy_;
    int noise_;
    CacheSet scatter_set_;
};
#endif // SCATTER_CACHE_H