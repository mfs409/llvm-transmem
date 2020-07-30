// CountingTM is a TM implementation designed for testing the correctness of
// the plugin.

// NB: Our tests do not always use this library correctly.  They may call clones
// directly, rather than use boundary instrumentation, and they make assumptions
// about how the compiler will generate code (and hence what should be counted).
// Thus countingTM is not a good reference implementation of TM in TM.
//
// In countingTM, all transactions are protected by a single global lock.
// Any instrumented memory access is achieved by performing the operation
// immediately.  However, on any memory access, we will increment the
// associated counter, so that we know how many instrumented calls happened.

#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <unordered_map>

#include "../../../common/tm_api.h"
#include "../include/tm_stats.h"

// Global metadata
namespace {
// The lock used for all concurrency control in countingTM.  Strictly speaking,
// this is a concurrency-safe TM.
std::mutex lock;

// Library-wide counts of library calls
//
// NB: be sure to hold the lock when accessing this array
int stats[TM_STATS_COUNT] = {0};

// The function pointer translation table
std::unordered_map<void *, void *> *clone_table = nullptr;
} // namespace

// Per-thread metadata
namespace {
// Each thread is assigned a TxThread object, so that it can track its
// transactional state
class TxThread {

  // construct a thread's transaction context by zeroing its nesting depth
  TxThread() : nesting(0) {}

  // The per-thread TxThread object
  static thread_local TxThread *self;

public:
  // nesting depth, if we call a transaction when inside a transaction
  int nesting;

  // Get the current thread's TxThread; build it if it doesn't exist.
  static TxThread *get_self() {
    if (__builtin_expect(TxThread::self == nullptr, false)) {
      TxThread::self = new TxThread();
    }
    return TxThread::self;
  }
};

// Provide backing for the per-thread TxThread
thread_local TxThread *TxThread::self = nullptr;
} // namespace

// Boundary instrumentation
//
// NB: these are not part of the public API
//
// Begin a transaction by acquiring the lock, unless this thread already has
// the lock, in which case we can just start running the (nested) transaction
void begin_tx() {
  // If nesting > 0, increment nesting and return immediately
  if (TxThread::get_self()->nesting++) {
    stats[TM_STATS_BEGIN_INNER]++;
    return;
  }
  // otherwise, get the lock before returning
  lock.lock();
  stats[TM_STATS_BEGIN_OUTER]++;
}

// Commit a transaction by releasing the lock, unless the thread is in a nested
// transaction, in which case we can just decrement the counter and return
void end_tx() {
  // If the decremented nesting depth is nonzero, we can just return
  if (--TxThread::get_self()->nesting) {
    stats[TM_STATS_END_INNER]++;
    return;
  }
  // otherwise, release the lock before returning
  stats[TM_STATS_END_OUTER]++;
  lock.unlock();
}

// The remaining portions of this file comprise the public API
extern "C" {
// In countingTM, unsafe-ness isn't a big deal... we just count the event and
// life goes on
void TM_UNSAFE() { stats[TM_STATS_UNSAFE]++; }

// Allocate memory by invoking the system malloc() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void *TM_MALLOC(size_t size) {
  // NB: can't do this or our tests will break
  // assert(TxThread::get_self()->nesting > 0);
  stats[TM_STATS_MALLOCS]++;
  return malloc(size);
}

// Allocate aligned memory by invoking the system aligned_alloc() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void *TM_ALIGNED_ALLOC(size_t alignment, size_t size) {
  stats[TM_STATS_ALIGNED_ALLOC]++;
  return aligned_alloc(alignment, size);
}

// Reclaim memory by invoking the system free() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void TM_FREE(void *ptr) {
  // NB: can't do this or our tests will break
  // assert(TxThread::get_self()->nesting > 0);
  stats[TM_STATS_FREES]++;
  free(ptr);
}

// Copy count characters from the object pointed to by src to the object
// pointed to by dest by invoking the system memcpy() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void *TM_MEMCPY(void *dest, const void *src, size_t count, size_t align) {
  stats[TM_STATS_MEMCPY]++;
  memcpy(dest, src, count);
  return dest;
}

// Sets the first num bytes of the block of memory pointed by ptr to the
// specified value
// by invoking the system memset() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void *TM_MEMSET(void *dest, int ch, size_t count) {
  stats[TM_STATS_MEMSET]++;
  memset(dest, ch, count);
  return dest;
}

// Copies count characters from the object pointed to by src to the object
// pointed to by dest
// by invoking the system memmove() function
//
// NB: we assume that the programmer does not manually insert calls to this
//     function, and hence it is always called while the lock is already held
void *TM_MEMMOVE(void *dest, const void *src, size_t count) {
  stats[TM_STATS_MEMMOVE]++;
  memmove(dest, src, count);
  return dest;
}

// Report the current value of a statistic that we've been counting
//
// NB: we assume that the programmer does not call this from *within* a
//     transaction.  To avoid races, we must acquire the lock before we can
//     access the stats array
int TM_STATS_GET(TM_STATS which) {
  int result = -1;
  lock.lock();
  if (which < TM_STATS_COUNT && which >= 0)
    result = stats[which];
  lock.unlock();
  return result;
}

// Reset all statistics
void TM_STATS_RESET() {
  for (int i = 0; i < TM_STATS_COUNT; ++i) {
    stats[i] = 0;
  }
}

// In order to easily create the 14 different load/store functions, we use the
// following macros.  In both cases, we increment a stat counter and access
// memory directly
#define LOADFUNC(TYPE, NAME, INDEX)                                            \
  __attribute__((always_inline)) TYPE NAME(TYPE *ptr) {                        \
    stats[INDEX]++;                                                            \
    return *ptr;                                                               \
  }
#define STOREFUNC(TYPE, NAME, INDEX)                                           \
  __attribute__((always_inline)) void NAME(TYPE val, TYPE *ptr) {              \
    stats[INDEX]++;                                                            \
    *ptr = val;                                                                \
  }

// Generate the load functions
LOADFUNC(uint8_t, TM_LOAD_U1, TM_STATS_LOAD_U1)
LOADFUNC(uint16_t, TM_LOAD_U2, TM_STATS_LOAD_U2)
LOADFUNC(uint32_t, TM_LOAD_U4, TM_STATS_LOAD_U4)
LOADFUNC(uint64_t, TM_LOAD_U8, TM_STATS_LOAD_U8)
LOADFUNC(float, TM_LOAD_F, TM_STATS_LOAD_F)
LOADFUNC(double, TM_LOAD_D, TM_STATS_LOAD_D)
LOADFUNC(long double, TM_LOAD_LD, TM_STATS_LOAD_LD)
LOADFUNC(void *, TM_LOAD_P, TM_STATS_LOAD_P)

// Generate the store functions
STOREFUNC(uint8_t, TM_STORE_U1, TM_STATS_STORE_U1)
STOREFUNC(uint16_t, TM_STORE_U2, TM_STATS_STORE_U2)
STOREFUNC(uint32_t, TM_STORE_U4, TM_STATS_STORE_U4)
STOREFUNC(uint64_t, TM_STORE_U8, TM_STATS_STORE_U8)
STOREFUNC(float, TM_STORE_F, TM_STATS_STORE_F)
STOREFUNC(double, TM_STORE_D, TM_STATS_STORE_D)
STOREFUNC(long double, TM_STORE_LD, TM_STATS_STORE_LD)
STOREFUNC(void *, TM_STORE_P, TM_STATS_STORE_P)

// For the C API, this is the call we actually make to launch an instrumented
// region
void TM_EXECUTE_C_INTERNAL(void (*func)(void *), void *args,
                           void (*anno_func)(void *)) {
  begin_tx();
  anno_func(args);
  end_tx();
}

// However, sometimes we still have to call the original function (e.g., if the
// plugin could not convert the argument to TM_EXECUTE_C)
void *TM_TRANSLATE_CALL(void *func);
void TM_EXECUTE_C(void *flags, void (*func)(void *), void *args) {
  // NB: C++ does not allow casting between function pointers and void*, so we
  //     use a union to do the dirty work
  union {
    void *voidstar;
    TM_C_FUNC cfunc;
  } clone;
  clone.voidstar = TM_TRANSLATE_CALL((void *)func);
  TM_EXECUTE_C_INTERNAL(func, args, clone.cfunc);
}

void TM_EXECUTE(void *flags, std::function<void(TM_OPAQUE *)> func) {
  begin_tx();
  try {
    func((TM_OPAQUE *)0xCAFE);
  } catch (...) {
    end_tx();
    throw;
  }
  end_tx();
}

// When the plugin cannot statically determine the clone of a function, it
// replaces the call to the uncloned function with a pair of instructions.  The
// First is a call to this, which takes as a parameter the address of the
// uncloned function.  The second is a call to the function returned by this
// function.  In that way, we can dynamically translate functions to their
// clones
//
// In countingTM, we also use this from TM_EXECUTE_C.  See below for concerns
void *TM_TRANSLATE_CALL(void *func) {
  if (clone_table == nullptr) {
    stats[TM_STATS_TRANSLATE_NOTFOUND]++;
    TM_UNSAFE();
    return func;
  }
  auto found = clone_table->find(func);
  if (found == clone_table->end()) {
    // if a clone is not found, we must make an unsafe call.
    //
    // NB: this is going to be complicated if we reach TM_UNSAFE from
    // TM_EXECUTE_C, because we won't be in a transaction yet.
    stats[TM_STATS_TRANSLATE_NOTFOUND]++;
    TM_UNSAFE();
    return func;
  } else {
    stats[TM_STATS_TRANSLATE_FOUND]++;
    return found->second;
  }
}

// When a shared library is loading, or when main begins, there will be calls to
// this function to add clone mappings to the clone table
void TM_REG_CLONE(void *from, void *to) {
  // We have some icky constructor-ordering issues that necessitate us
  // making the clone_table a pointer so that we can manually initialize it
  if (clone_table == nullptr)
    clone_table = new std::unordered_map<void *, void *>();
  // NB: this is not thread safe, but it needs to be!
  clone_table->insert({from, to});
  stats[TM_STATS_REGCLONE]++;
}

void TM_REPORT_ALL_STATS() {
  using std::cout;
  using std::endl;
  for (size_t i = 0; i < TM_STATS_COUNT; ++i) {
    cout << TM_STAT_NAMES[i] << " : " << stats[i] << endl;
  }
}

bool TM_RAII_BEGIN(jmp_buf & buffer) {
  begin_tx();
  return true;
}

void TM_RAII_END() { end_tx(); }
}
