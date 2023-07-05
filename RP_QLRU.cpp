#include "RP_LRU.h"
#include "RP_QLRU.h"

#include "assert.h"

// AKA replIndexFun
unsigned RP_QLRU::insertionPosition(unsigned set, unsigned slice)
{
  // check for invalid lines
  // if r0, r1, search from front to back, if r1 from back to front
  for (int i = (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? 0 : ways_ - 1;
    (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? ((unsigned) i < ways_) : (i >= 0);
    (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? i++ : i--)
  {
    if (!memory_[slice * lines_ + set * ways_ + i].valid)
      return i;
  }

  switch (qlru_repl_idx_)
  {
    case R0:
    case R2:
    {
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if (memory_[slice * lines_ + set * ways_ + i].time == 3)
          return i;
      }
      break;
    }
    case R1:
    {
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if (memory_[slice * lines_ + set * ways_ + i].time == 3)
          return i;
      }
      return 0;
    }
  }


  printf("Undefined behaviour\n");
  return rand() % ways_;
}


// if no invalid position is found, ways_ is returned (aka invalid way -> insertion policy)
void RP_QLRU::findInvalid(CacheSet* set, unsigned slice)
{
  // Search for invalid
  for (subset_ = 0; subset_ < subsets_; subset_++)
  {
    for (way_ = (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? 0 : subset_ways_ - 1;
      (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? ((unsigned)way_ < subset_ways_) : (way_ >= 0);
      (qlru_repl_idx_ == R0 || qlru_repl_idx_ == R1) ? way_++ : way_--)
    {
      if (!(*c_memory_)(slice, subset_, set, way_).valid)
        return;
    }
  }

  subset_ = 0;
  if (subsets_ != 1)
    subset_ = rand() % subsets_;

  way_ = ways_;
}

InsertionPosition RP_QLRU::insertionPosition(CacheSet* set, unsigned slice)
{
  if (!umo_)
    findInvalid(set, slice);

  if (way_ < ways_)
    return { subset_, way_ };
  
  switch (qlru_repl_idx_)
  {
    case R0:
    case R2:
    {
      for (int i = 0; (unsigned) i < subset_ways_; i++)
      {
        if ((*c_memory_)(slice, subset_, set, i).time == 3)
          return { subset_, (unsigned)i };
      }
      break;
    }
    case R1:
    {
      for (int i = 0; (unsigned) i < subset_ways_; i++)
      {
        if ((*c_memory_)(slice, subset_, set, i).time == 3)
          return { subset_, (unsigned)i };
      }

      return { subset_, 0 };
    }
  }

  printf("Undefined behaviour\n");
  return { subset_, rand() % subset_ways_ };
}

// this is only executed if umo, which is only executed if NO HIT
unsigned RP_QLRU::updateAndInsert(unsigned set, unsigned slice)
{
  updateFun(set, slice, ways_);
  return insertionPosition(set, slice);
}

// this is only executed if umo, which is only executed if NO HIT
InsertionPosition RP_QLRU::updateAndInsert(CacheLine** line, CacheSet* set, unsigned slice)
{
  // in order to know the subset for updateFun, we need to first get it from an 
  // invalid line or calculate it randomly, findInvalid is otherwise called in insertionPosition 
  // if umo_ is false.
  findInvalid(set, slice);
  updateFun(set, slice, subset_, ways_);

  InsertionPosition ins_pos = insertionPosition(set, slice);
  *line = &(*c_memory_)(slice, ins_pos.subset, set, ins_pos.way);

  return ins_pos;
}

void RP_QLRU::updateSet(unsigned set, unsigned slice, unsigned way, bool replacement)
{
  unsigned time = memory_[slice * lines_ + set * ways_ + way].time;
  memory_[slice * lines_ + set * ways_ + way].time = replacement ? missFun() : hitFun(time);
  if (!umo_)
    updateFun(set, slice, way);
}

void RP_QLRU::updateSet(CacheLine& line, CacheSet* set, unsigned slice, unsigned subset, unsigned way, bool replacement)
{
  unsigned time = line.time;
  line.time = replacement ? missFun() : hitFun(time);
  if (!umo_)
    updateFun(set, slice, subset, way);
}

unsigned RP_QLRU::hitFun(unsigned time) {
  switch(time)
  {
    case 2:
    {
      switch (qlru_hit_)
      {
        case H11:
        case H21:
          return 1;
        default:
          return 0;
      }
      assert(false);
    }
    case 3:
    {
      switch (qlru_hit_)
      {
        case H11:
        case H10:
          return 1;
        case H20:
        case H21:
          return 2;
        default:
          return 0;
      }
      assert(false);
    }
    default:
      return 0;
  }
  assert(false);
}

unsigned RP_QLRU::missFun() {
  return (unsigned) qlru_miss_;
}

void RP_QLRU::updateFun(unsigned set, unsigned slice, unsigned way)
{
  switch (qlru_upd_)
  {
    case U0:
    {
      int c;
      int max_time = 0;
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if ((c = memory_[slice * lines_ + set * ways_ + i].time) > max_time)
          max_time = c;
      }
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        memory_[slice * lines_ + set * ways_ + i].time = memory_[slice * lines_ + set * ways_ + i].time + (3 - max_time);
      }
      break;
    }
    case U1:
    {
      unsigned c;
      unsigned max_time = 0;
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if ((unsigned) i != way && (c = memory_[slice * lines_ + set * ways_ + i].time) > max_time)
          max_time = c;
      }
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if ((unsigned) i != way)
          memory_[slice * lines_ + set * ways_ + i].time = memory_[slice * lines_ + set * ways_ + i].time + (3 - max_time);
      }
      break;
    }
    case U2:
    {
      for (int i = 0; (unsigned)i < ways_; i++)
      {
        if (memory_[slice * lines_ + set * ways_ + i].time == 3)
        {
          return;
        }
      }
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        memory_[slice * lines_ + set * ways_ + i].time++;
      }
      break;
    }
    case U3:
    {
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if (memory_[slice * lines_ + set * ways_ + i].time == 3)
        {
          return;
        }
      }
      for (int i = 0; (unsigned) i < ways_; i++)
      {
        if ((unsigned) i != way)
          memory_[slice * lines_ + set * ways_ + i].time++;
      }
      break;
    }
  }
}

void RP_QLRU::updateFun(CacheSet* set, unsigned slice, unsigned subset, unsigned way)
{
  switch (qlru_upd_)
  {
    case U0:
    {
      int c;
      int max_time = 0;
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((c = (*c_memory_)(slice, subset, set, i).time) > max_time)
          max_time = c;
      }
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        (*c_memory_)(slice, subset, set, i).time = (*c_memory_)(slice, subset, set, i).time + (3 - max_time);
      }
      break;
    }
    case U1:
    {
      unsigned c;
      unsigned max_time = 0;
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((unsigned)i != way && (c = (*c_memory_)(slice, subset, set, i).time) > max_time)
          max_time = c;
      }
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((unsigned)i != way)
          (*c_memory_)(slice, subset, set, i).time = (*c_memory_)(slice, subset, set, i).time + (3 - max_time);
      }
      break;
    }
    case U2:
    {
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((*c_memory_)(slice, subset, set, i).time == 3)
        {
          return;
        }
      }
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        (*c_memory_)(slice, subset, set, i).time++;
      }
      break;
    }
    case U3:
    {
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((*c_memory_)(slice, subset, set, i).time == 3)
        {
          return;
        }
      }
      for (int i = 0; (unsigned)i < subset_ways_; i++)
      {
        if ((unsigned)i != way)
        {
          (*c_memory_)(slice, subset, set, i).time++;
        }
      }
      break;
    }
  }
}

RPolicy* RP_QLRU::getCopy()
{
    return static_cast<RPolicy*>(new RP_QLRU(*this));
}