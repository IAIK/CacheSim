## Cache Types

### Basic Caches
These caches are objects that represent a single cache (possibly with slices)
- **Cache**  
  all other caches derive from this abstract class
- **SACache**  
  Standard set-associattive cache that can use the policies defined below.  
  Faster than GenericCache as it's single-purpose.
- **GenericCache**  
  This cache implements the model defined by Purnal et al. in the paper [Systematic Analysis of Randomization-based Protected Cache Architectures](https://ginerlukas.com/publications/papers/SystematicAnalysis2020.pdf)  
  By using the mapping functions below, it can implement a large variety of randomized and standard cache designs.
- **ScatterCache**  
  Old implementation of [ScatterCache](https://www.usenix.org/system/files/sec19-werner.pdf), uses SHA256 and is therefore slow. Superseded by GenericCache, see below.
### Combined Caches
These caches represent collections or modifiers of basic caches

- **SplitCache**  
  Takes 2 basic caches and treats one as an instruction cache and the other as the the data cache.  
  Accesses are split between the two according to the instruction flag for accesses.
- **CacheHierarchy**  
  Takes *n* caches and places them in a hierarchy.  
  Currently only the inclusive mode is reasonably well tested.
  #### inclusive mode:
  Read accesses are served from the lowest cache level possible and cached in lower levels along the way.  
  Writes are propagated all the way to the top level regardless of hits on lower levels and update all replacement policies.  
  Evictions in higher-level caches trigger flushes in lower level caches to maintain inclusivity.
- **NoisyCache**  
  Adds a defined amount of noise accesses for every normal access.  
  Currently relies on security domains for randomness of the noise, should not be used with non-randomized caches as it simply accesses address linearly from 0.

## Replacement Policies

Replacement policy objects can be used with SACache and GenericCache.

- **RP_LRU**  
  Standard LRU.
- **RP_PLRU**  
  [Pseudo PLRU](https://en.wikipedia.org/wiki/Pseudo-LRU), Tree-PLRU implementation as used in modern CPUs.
- **RP_QLRU**  
  QLRU implementation as defined in [nanoBench: A Low-Overhead Tool for Running Microbenchmarks on x86 Systems](https://arxiv.org/pdf/1911.03282.pdf).
- **RP_BIP**  
  Bimodal Insertion Policy implementation as defined in [Adaptive Insertion Policies for High Performance Caching](https://dl.acm.org/doi/abs/10.1145/1273440.1250709).
- **RP_RANDOM**  
  Random insertion.

  Replacement policies typically a CacheConfig (see below):

```c++
    CacheConfig l1_config = {cache_line_bits, cache_way_bits, cache_slices, cache_line_size_bits};
    RP_PLRU l1_policy(l1_config);
```

## Mapping Functions

These objects implement the mapping function R for GenericCache.

- **RMapper**  
  Default linear mapping from address to set.
  Implements standard set-associattive cache behaviour.
- **RAES_NI**  
  Randomized mapping with AES NI hardware instructions  
  Can implement [ScatterCache](https://www.usenix.org/system/files/sec19-werner.pdf), [CEASER-S](https://dl.acm.org/doi/abs/10.1145/3307650.3322246) and similar
- **RSHA256**  
  Randomized mapping based on a SHA256 software implementation.
- **RSASS_AES_NI**  
  Randomized scattered mapping with AES NI hardware instructions.
  Implements [SassCache](https://ginerlukas.com/publications/papers/sass.pdf).
- **RCAT**  
  Implements a partitioned cache that is split along the ways like [Intel CAT](https://www.intel.com/content/www/us/en/developer/articles/technical/introduction-to-cache-allocation-technology.html).


## CacheConfig

All basic caches take a CacheConfig as an argument:
```c++
    struct CacheConfig {
      unsigned line_bits;
      unsigned way_bits;
      unsigned slices;
      unsigned line_size_bits;
    };
```

The parameters are given in bits, as only powers of 2 are currently supported.



## GenericCache

The generic cache is intialized according to the model by Purnal et al.  
The arguments are a CacheConfig object, a policy object, an R object and the number of partitions P.  
It is important that the parameters shared by the R object and the cache (config and p) are the same.
For example, a standard set assosciative cache can be set up like this:  

```c++
    CacheConfig config = {cache_line_bits, cache_way_bits, cache_slices, cache_line_size_bits};
    RP_PLRU policy(config);
    unsigned P = 1;
    RMapper R(config, P, 0);
    Cache *g_cache = new GenericCache(config, policy, R, P);
```
  
ScatterCache can be set up like this:  

```c++
    CacheConfig config = {cache_line_bits, cache_way_bits, cache_slices, cache_line_size_bits};

    //P = n_ways for full scattering (scattercache and sass)
    unsigned P = 1 << (config.way_bits);

    RP_RANDOM policy_random(config);

    RAES_NI R_AES(config, P, ((uint64_t)rand()<<32) + (uint64_t)rand()); //generate a random secret
    Cache *scattercache = new GenericCache(config, policy_random, R_AES, P)
```
## Tests
Test examples are provided as is and could be partially outdated due to changes in the codebase, though they should compile.
They can be used as a reference how to use various features.

Build with `make attack_sim` (without .cpp), run with `attack_sim.x`.



## misc

make clean and prepend DEBUG=1 to make to enable assert sanity checks