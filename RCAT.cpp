#include <assert.h>
#include <math.h>

#include "RCAT.h"

RCAT::RCAT(CacheConfig config, unsigned subsets, unsigned domains) : RMapper(config, subsets), domains_(domains)
{
  name_ = "CAT";
  domain_sets_  = sets_/domains_;

  assert(__builtin_popcount(domains_) == 1);
  assert(domain_sets_*domains == sets_);

}

//partition the cache along the sets.
//equals way partition if ways are divided by domains in CacheConfig
CacheSet *RCAT::getIndices(uint64_t phys_addr, uint64_t domain)
{
  assert(domain < domains_);
  CacheSet *set = RMapper::getIndices(phys_addr, domain);

  for (unsigned i = 0; i < subsets_; i++)
    set->index[i] = set->index[i] % domain_sets_ + domain_sets_*domain;

  return set;
}

RMapper* RCAT::getCopy()
{
   return static_cast<RMapper*>(new RCAT(*this));
}

RCAT::~RCAT()
{
}
