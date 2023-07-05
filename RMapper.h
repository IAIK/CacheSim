#ifndef R_MAPPING_H
#define R_MAPPING_H

#include <cstdint>
#include <string>

#include "Cache.h"

typedef enum {SLICING_STATIC} SliceFunction;

class RMapper
{
  public:
    RMapper(CacheConfig config, unsigned P, uint64_t key = 0);

    virtual CacheSet *getIndices(size_t phys_addr, uint64_t secret);
    virtual unsigned getSlice(size_t phys_addr);
    virtual RMapper* getCopy();

    string getMapperName() { return name_; };

    virtual ~RMapper();
  
  public:
    unsigned line_size_;
    unsigned line_size_bits_;
    uint32_t lines_;
    unsigned line_bits_;
    unsigned ways_;
    unsigned way_bits_;
    unsigned slices_;
    uint32_t size_;
    unsigned sets_;
    unsigned subsets_;
    unsigned subset_bits_;
    unsigned subset_ways_;
    string name_;
    CacheSet set_;

    uint64_t key_;
};
#endif // R_MAPPING_H