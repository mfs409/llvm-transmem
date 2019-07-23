/// p_alloc.h provides a set of allocation managers that can be used by a PTM
/// implementation.  These allocation managers all provide the same public
/// interface, so that they are interchangeable in PTM algorithms.
///
/// NB: We assume that there is some amount of recoverability in the system
///     allocator.  However, the AllocationManager object still needs to have
///     some amount of persistence to it.  We can assume that the recovery
///     algorithm will walk the heap and do reachability to reconstruct the free
///     list.  As a result, the AllocationManager object does not need to have a
///     persistent free list... if a transaction put something in its free list,
///     then it *should have* also performed enough writes to make that memory
///     unreachable.  If the system crashes before the transaction commits, then
///     during recovery run the undo log before reconstructing the freelist.
///     Since the frees didn't happen anyway, the data will be reachable, and
///     all is good.  If the system crashes after the transaction commits, then
///     first run the redo log.  Free'd data should then be unreachable, and
///     thus will be treated as free.
///
///     However, we need to handle mallocs.  When does a malloc need to be
///     undone?  It needs to be undone if the system crashes before the
///     transaction marks itself as done.  So, obviously, the list needs to be
///     persistent.  But that's not enough.  For an abort, we run the undo log,
///     then reclaim all in the malloc list.  For a commit, we run the redo log,
///     and then drop the malloc list.  But we have a fast-path for read-only
///     transactions, where they never toggle their state.  Can a read-only
///     transaction malloc?  Maybe.  It's likely that the transaction would have
///     to write a reference to its new memory *somewhere*, and wouldn't be
///     read-only.  If the system crashes when a transaction's state is
///     "inactive", but the malloc list is not empty, then we say that the
///     transaction did not commit, and we reclaim the mallocs.  If the list is
///     empty, then we say the transaction did commit, and we don't reclaim.  So
///     really, p_clear() is the point of durable linearizability for read-only
///     transactions that perform allocations.  This is especially subtle since
///     there will be p_precommit() calls.  But it appears to be sufficient for
///     recoverability, and also to ensure we incur all of the fence overheads
///     that one might expect.
///
///     The above logic does require us to split onCommit into commitMallocs and
///     commitFrees, because commitFrees must be after quiescence, whereas
///     commitMallocs must be before marking self complete.
#pragma once

#include <cstdlib>

#include "minivector.h"
#include "p_minivector.h"
#include "platform.h"
#include "pmem_platform.h"

/// The BasicPersistentAllocationManager provides a mechanism for a transaction
/// to log its allocations and frees, and to finalize or undo them if the
/// transaction commits or aborts.
class BasicPersistentAllocationManager {
protected:
  /// a list of all not-yet-committed allocations in this transaction
  P_MiniVector<void *> mallocs;

  /// a list of all not-yet-committed deallocations in this transaction
  MiniVector<void *> frees;

  /// Track if allocation management is active or not
  bool active = false;

public:
  /// Indicate that logging should begin
  void onBegin() { active = true; }

  /// When a transaction commits, finalize its mallocs.  Note that this should
  /// be called *before* the transaction's durable linearization point
  void commitMallocs() { mallocs.p_clear(); }

  /// When a transaction commits, finalize its frees.  Note that
  /// this should be called *after* privatization is ensured.
  void commitFrees() {
    for (auto a : frees) {
      free(a);
    }
    frees.clear();
    active = false;
  }

  /// When a transaction aborts, drop its frees and reclaim its mallocs
  void onAbort() {
    frees.clear();
    for (auto p : mallocs) {
      free(p.val);
    }
    mallocs.p_clear();
    active = false;
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  void *alloc(size_t size) {
    void *res = malloc(size);
    if (active) {
      mallocs.push_back(res);
    }
    return res;
  }

  /// Allocate memory that is aligned on a byte boundary as specified by A
  void *alignAlloc(size_t A, size_t size) {
    void *res = aligned_alloc(A, size);
    if (active) {
      mallocs.push_back(res);
    }
    return res;
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void reclaim(void *addr) {
    if (active) {
      frees.push_back(addr);
    } else {
      free(addr);
    }
  }

  /// Capture optimization is not supported
  bool checkCaptured(void *addr) { return false; }

  /// Since capture is not supported, capture flushing is not required
  void p_flushOnCapturedStore(void *addr) {}

  /// Before committing, flush all stores to captured memory (no-op for this
  /// allocation manager)
  bool p_precommit() { return false; }
};

/// The EnhancedPersistentAllocationManager does everything that
/// BasicPersistentAllocationManager does, and adds the dynamic captured memory
/// optimization. This optimization tracks the most recent allocation and
/// suggests to the TM that accesses in that allocation shouldn't be
/// instrumented.  Note that more work is needed than in the STM case, because
/// at commit time we must flush all updates to all malloc'd regions, in case
/// they were treated as captured.
class EnhancedPersistentAllocationManager {
protected:
  /// a list of all not-yet-committed allocations in this transaction
  P_MiniVector<std::pair<void *, size_t>> mallocs;

  /// a list of all not-yet-committed deallocations in this transaction
  MiniVector<void *> frees;

  /// Track if allocation management is active or not
  bool active = false;

  /// Address of last allocation
  void *lastAlloc;

  /// Size of last allocation
  size_t lastSize;

public:
  /// Indicate that logging should begin
  void onBegin() { active = true; }

  /// When a transaction commits, finalize its mallocs.  Note that this should
  /// be called *before* the transaction's durable linearization point
  void commitMallocs() { mallocs.p_clear(); }

  /// When a transaction commits, finalize its frees.  Note that
  /// this should be called *after* privatization is ensured.
  void commitFrees() {
    for (auto a : frees) {
      free(a);
    }
    frees.clear();
    active = false;
    lastAlloc = nullptr;
    lastSize = 0;
  }

  /// When a transaction aborts, drop its frees and reclaim its mallocs
  void onAbort() {
    frees.clear();
    for (auto p : mallocs) {
      free(p.val.first);
    }
    mallocs.p_clear();
    active = false;
    lastAlloc = nullptr;
    lastSize = 0;
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  ///
  /// NB: the function pointer is ignored in this allocation manager
  void *alloc(size_t size) {
    void *res = malloc(size);
    if (active) {
      mallocs.push_back({res, size});
      lastAlloc = res;
      lastSize = size;
    }
    return res;
  }

  /// Allocate memory that is aligned on a byte boundary as specified by A
  ///
  /// NB: the function pointer is ignored in this allocation manager
  void *alignAlloc(size_t A, size_t size) {
    void *res = aligned_alloc(A, size);
    if (active) {
      mallocs.push_back({res, size});
      lastAlloc = res;
      lastSize = size;
    }
    return res;
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void reclaim(void *addr) {
    if (active) {
      frees.push_back(addr);
    } else {
      free(addr);
    }
  }

  /// Return true if the given address is within the range returned by the most
  /// recent allocation
  bool checkCaptured(void *addr) {
    uintptr_t lstart = (uintptr_t)lastAlloc;
    uintptr_t lend = lstart + lastSize;
    uintptr_t a = (uintptr_t)addr;
    return a >= lstart && a < lend;
  }

  /// Since we use p_precommit, captured memory does not need a flush at the
  /// time when it is written.  The clwbs will happen in p_precommit()
  void p_flushOnCapturedStore(void *addr) {}

  /// Before committing, flush all stores to captured memory
  bool p_precommit() {
    if (mallocs.size() == 0) {
      return false;
    }
    // For each ptr/size pair, flush the range from ptr to ptr + size
    for (auto p = mallocs.begin(), e = mallocs.end(); p != e; ++p) {
      for (size_t i = 0; i < p->val.second; i += CACHELINE_BYTES) {
        pmem_clwb(((char *)p->val.first) + i);
      }
    }
    return true;
  }
};

/// The NaiveCapturingPersistentAllocationManager exists only for the sake of
/// measurement.  It is like EnhancedPersistentAllocationManager, in that it
/// instructs the TM not to do concurrency instrumentation on captured memory.
/// However, it *does not manage deferred flushing*.  Instead, it lets calling
/// code (i.e., TxThread::write()) know when it has captured a location, so that
/// code can issue a clwb.
class NaiveCapturingPersistentAllocationManager {
protected:
  /// a list of all not-yet-committed allocations in this transaction
  P_MiniVector<void *> mallocs;

  /// a list of all not-yet-committed deallocations in this transaction
  MiniVector<void *> frees;

  /// Track if allocation management is active or not
  bool active = false;

  /// Address of last allocation
  void *lastAlloc;

  /// Size of last allocation
  size_t lastSize;

public:
  /// Indicate that logging should begin
  void onBegin() { active = true; }

  /// When a transaction commits, finalize its mallocs.  Note that this should
  /// be called *before* the transaction's durable linearization point
  void commitMallocs() { mallocs.p_clear(); }

  /// When a transaction commits, finalize its frees.  Note that
  /// this should be called *after* privatization is ensured.
  void commitFrees() {
    for (auto a : frees) {
      free(a);
    }
    frees.clear();
    active = false;
    lastAlloc = nullptr;
    lastSize = 0;
  }

  /// When a transaction aborts, drop its frees and reclaim its mallocs
  void onAbort() {
    frees.clear();
    for (auto p : mallocs) {
      free(p.val);
    }
    mallocs.p_clear();
    active = false;
    lastAlloc = nullptr;
    lastSize = 0;
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  ///
  /// NB: the function pointer is ignored in this allocation manager
  void *alloc(size_t size) {
    void *res = malloc(size);
    if (active) {
      mallocs.push_back(res);
      lastAlloc = res;
      lastSize = size;
    }
    return res;
  }

  /// Allocate memory that is aligned on a byte boundary as specified by A
  ///
  /// NB: the function pointer is ignored in this allocation manager
  void *alignAlloc(size_t A, size_t size) {
    void *res = aligned_alloc(A, size);
    if (active) {
      mallocs.push_back(res);
      lastAlloc = res;
      lastSize = size;
    }
    return res;
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void reclaim(void *addr) {
    if (active) {
      frees.push_back(addr);
    } else {
      free(addr);
    }
  }

  /// Return true if the given address is within the range returned by the most
  /// recent allocation
  bool checkCaptured(void *addr) {
    uintptr_t lstart = (uintptr_t)lastAlloc;
    uintptr_t lend = lstart + lastSize;
    uintptr_t a = (uintptr_t)addr;
    return a >= lstart && a < lend;
  }

  /// This allocator does not use deferred flushing of captured memory, so after
  /// a write-to-captured, it must perform the clwb for the TM
  void p_flushOnCapturedStore(void *addr) {
    if (checkCaptured(addr)) {
      pmem_clwb(addr);
    }
  }

  /// This allocator reqquires the TM to clwb any stores to captured memory at
  /// the time of the store, rather than doing the clwbs at commit time.
  /// However, if the very last store of a transaction was to captured memory,
  /// then the store's clwb may need to be ordered before lock release.  We can
  /// approximate by saying "if there were any mallocs, then return true so that
  /// the TM will fence before releasing locks".
  bool p_precommit() { return mallocs.size() > 0; }
};
