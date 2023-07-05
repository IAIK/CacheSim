#ifndef CODE_RP_QLRU_H
#define CODE_RP_QLRU_H

#include "RPolicy.h"

// SOURCES:
// https://github.com/andreas-abel/nanoBench/blob/master/tools/CacheAnalyzer/cacheSim.py
// https://uops.info/cache.html

using std::to_string;

typedef enum { H00, H10, H11, H20, H21 } QLRUHitFunc;
typedef enum { M0, M1, M2, M3 } QLRUMissFunc;
typedef enum { R0, R1, R2 } QLRUReplIdxFunc;
typedef enum { U0, U1, U2, U3 } QLRUUpdFunc;

struct QLRUConfig {
  QLRUHitFunc hitFun;
  QLRUMissFunc missFun;
  QLRUReplIdxFunc idxFun;
  QLRUUpdFunc uptFun;
  bool updateOnlyOnMiss=false;
};

class RP_QLRU : public RPolicy
{
  public:
    RP_QLRU(CacheConfig config, QLRUConfig qlru_config) : RPolicy(config)
    {
        qlru_hit_ = qlru_config.hitFun;
        qlru_miss_ = qlru_config.missFun;
        qlru_repl_idx_ = qlru_config.idxFun;
        qlru_upd_ = qlru_config.uptFun;
        umo_ = qlru_config.updateOnlyOnMiss;

        name_ = "RP_QLRU_" + to_string(qlru_hit_) + "_" + to_string(qlru_miss_) + "_" + to_string(qlru_repl_idx_) + "_"
            + to_string(qlru_upd_) + (umo_ ? "_UMO" : "");
    };
    ~RP_QLRU() override = default;
    
    unsigned insertionPosition(unsigned set, unsigned slice) override;
    InsertionPosition insertionPosition(CacheSet* set, unsigned slice) override;

    unsigned updateAndInsert(unsigned set, unsigned slice) override;
    InsertionPosition updateAndInsert(CacheLine** line, CacheSet* set, unsigned slice) override;
    
    void updateSet(unsigned set, unsigned slice, unsigned way, bool replacement) override;
    void updateSet(CacheLine& line, CacheSet *set, unsigned slice, unsigned subset, unsigned way, bool replacement) override;

    // this needs to be called at initialization of the cache aswell!
    void reset() override
    {
      for (unsigned i = 0; i < lines_; i++)
        memory_[i].time = 3;
    };
    RPolicy* getCopy() override;

  private:
    QLRUHitFunc qlru_hit_;
    QLRUMissFunc qlru_miss_;
    QLRUReplIdxFunc qlru_repl_idx_;
    QLRUUpdFunc qlru_upd_;

    // these 3 are used for genericcache only
    unsigned subset_;
    unsigned way_;
    void findInvalid(CacheSet* set, unsigned slice);

    unsigned hitFun(unsigned hit);
    unsigned missFun();

    void updateFun(unsigned set, unsigned slice, unsigned way);
    void updateFun(CacheSet* set, unsigned slice, unsigned subset, unsigned way);
};


#endif //CODE_RP_QLRU_H
