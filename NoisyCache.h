#ifndef NOISY_CACHE_H
#define NOISY_CACHE_H

#include <math.h>
#include <time.h>

#include "Cache.h"


class NoisyCache : public Cache
{
  public:

    NoisyCache(Cache *cache, uint64_t noise_secret, double noise_level) : cache_(cache), noise_secret_(noise_secret), noise_level_(noise_level)
    {
      if (noise_level == 0)
      {
        noise_ = false;
        return;
      }
      srand(time(NULL));
      float intpart;
      double fractpart = modf(noise_level , &intpart);
      noise_int_ = (unsigned) intpart;
      noise_frac_bar_ = RAND_MAX*fractpart;
      //printf ("%f = %u + %u \n", noise_level, noise_int_, noise_frac_bar_);
    };

    void clearCache()
    {
      cache_->clearCache();
    };
    CLState isCached(size_t addr, size_t secret)
    {
      return cache_->isCached(addr, secret);
    };
    AccessResult access(Access mem_access)
    {
      if (noise_)
      {
        for (unsigned i = 0; i < noise_int_; i++)
        {
          current_addr += cache_->getLineSize();
          cache_->access({current_addr, noise_secret_, 0, 0});
          noise_accesses_++;
        }
        if (rand() < noise_frac_bar_)
        {
          current_addr += cache_->getLineSize();
          cache_->access({current_addr, noise_secret_, 0, 0});
          noise_accesses_++;
        }
      }

      return cache_->access(mem_access);
    };
    void resetUsage(int slice)
    {
      cache_->resetUsage(slice);
    };
    void flush(Access mem_access)
    {
      cache_->flush(mem_access);
    };
    uint32_t getUsage(int slice)
    {
      return cache_->getUsage(slice);
    };
    uint64_t hits(int slice)
    {
      return cache_->hits(0);
    };
    uint64_t misses(int slice)
    {
      return cache_->misses(0);
    };
    unsigned getSlice(size_t phys_addr)
    {
      return cache_->getSlice(phys_addr);
    };
    unsigned getCacheSize()
    {
      return cache_->getCacheSize();
    };
    uint64_t getLineSize()
    {
      return cache_->getLineSize();
    };
    unsigned getLines()
    {
      return cache_->getLines();
    };
    unsigned getWays()
    {
      return cache_->getWays();
    };
    string getName()
    {
      return cache_->getName();
    };
    string getPolicy()
    {
      return cache_->getPolicy();
    };
    unsigned getSlices()
    {
      return cache_->getSlices();
    };

    ~NoisyCache()
    {

    };

    uint64_t noise_accesses_ = 0;
  
  private:

    Cache *cache_;
    uint64_t noise_secret_;
    double noise_level_;

    bool noise_ = true;
    unsigned noise_int_;
    int noise_frac_bar_;

    uint64_t current_addr = 0;
    
};
#endif // NOISY_CACHE_H