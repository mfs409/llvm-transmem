#pragma once

#include <cstdlib>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/p_status_t.h"

/// P_CGL_Lazy is an PTM algorithm with the following characteristics:
/// - All transactions are protected by a single coarse-grained lock, and thus
///   There is no concurrency among transactions.
/// - All writes are deferred until commit time.
///
/// P_CGL_Lazy can be customized in the following ways:
/// - Lock implementation (spin lock, std::mutex, etc)
/// - Stack Frame (only used to manage transaction nesting)
/// - Allocator (to standardize malloc/free behavior across implementations)
/// - PMEM durability domain (ADR or eADR)
template <class LOCK, class REDOLOG, class STACKFRAME, class ALLOCATOR,
          class PMEM>
class P_CGL_Lazy {
  /// Globals is a wrapper around all of the global variables used by CGL
  struct Globals {
    /// The lock used for all concurrency control.
    LOCK lock;

    /// Certain Redo Log implementations require a total order on all commits.
    /// We implicitly have that order.  This makes it explicit.
    uint64_t commit_order = 0;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// The transaction's persistent status
  P_status<PMEM> p_status;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// The transaction's undo log, in case of crashes
  REDOLOG p_redolog;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// Return the irrevocability state of the thread.  It's false, because we
  /// require instrumentation.
  bool isIrrevoc() { return false; }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) { frame.setBottom(addr); }

  /// construct a thread's transaction context
  P_CGL_Lazy() : p_status(nullptr, &p_redolog, &allocator) {}

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx(void *stack_addr) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      frame.setBottom(stack_addr);
      globals.lock.acquire();
      // Start logging allocations
      allocator.onBegin();
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      // Read-only: we never were visible so cleanup is easy...
      if (p_redolog.isEmpty()) {
        // possibly flush writes to captured memory
        if (allocator.p_precommit()) {
          PMEM::sfence();
        }
        allocator.commitMallocs();
        globals.lock.release();
        allocator.commitFrees();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // Writer transaction
      // clwb any stores to captured memory, and persist the redo log
      allocator.p_precommit();
      p_redolog.p_precommit();
      PMEM::sfence(); // NB: always needed... see redolog precommit
      // Mark self committed, according to redolog's writeback mode
      if (!p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        // Since writeback does clwb while holding lock, we set self to REDO
        p_status.before_redo_writeback();
      } else {
        // Since writeback doesn't clwb, we set self to a timestamp
        p_status.before_redo_writeback_opt(++globals.commit_order);
      }
      // writeback, mark done, release lock, post-writeback
      //
      // NB: if we have delayed clwbs of program memory, then we can't mark self
      //     done until *after* the clwbs.  p_postcommit() will be a nop in that
      //     case.
      p_redolog.p_writeback();
      allocator.commitMallocs(); // has a fence
      if (!p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        p_status.after_redo_writeback();
      }
      globals.lock.release();
      p_redolog.p_postcommit();
      if (p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        p_status.after_redo_writeback();
      }

      // clean up
      p_redolog.clear();
      allocator.commitFrees();
      deferredActions.onCommit();
      frame.onCommit();
    }
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  void *txAlloc(size_t size) { return allocator.alloc(size); }

  /// To allocate aligned memory, we must also log it, so we can reclaim it if
  /// the transaction aborts
  void *txAAlloc(size_t A, size_t size) {
    return allocator.alignAlloc(A, size);
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void txFree(void *addr) { allocator.reclaim(addr); }

  /// Transactional read:
  ///
  /// NB: we assume that a data type is never aligned such that it crosses a
  /// 64-byte boundary, since that is the granularity of redo log entries.
  template <typename T> T read(T *ptr) {
    // No instrumentation if on stack or captured
    if (accessDirectly(ptr)) {
      return *ptr;
    }

    // Lookup in redo log to populate ret.  Note that prior casting can lead to
    // ret having only some bytes properly set
    T ret;
    typename REDOLOG::mask_t found_mask = p_redolog.get_partial(ptr, ret);
    // If we found all the bytes in the redo log, then it's easy
    typename REDOLOG::mask_t desired_mask = REDOLOG::create_mask(sizeof(T));
    if (desired_mask == found_mask) {
      return ret;
    }

    // We need to get at least some of the bytes from main memory
    T from_mem = *ptr;

    // If redolog was a partial hit, reconstruction is needed
    if (!found_mask) {
      return from_mem;
    }
    REDOLOG::reconstruct(from_mem, ret, found_mask);
    return ret;
  }

  /// Transactional write
  template <typename T> void write(T *ptr, T val) {
    // No instrumentation if on stack or captured
    if (accessDirectly(ptr)) {
      *ptr = val;
      allocator.p_flushOnCapturedStore(ptr);
      return;
    }
    // Otherwise, simply insert into the redo log
    if (p_redolog.insert_if(ptr, val)) {
      return;
    }
    // Read enough to fill the redolog chunk
    uint64_t fill[REDOLOG::REDO_WORDS];
    uintptr_t *start_addr =
        (uintptr_t *)(((uintptr_t)ptr) & ~(REDOLOG::REDO_CHUNKSIZE - 1));
    memcpy(&fill, start_addr, REDOLOG::REDO_CHUNKSIZE);
    // Insert into the redo log, including the consistent read of fill
    p_redolog.insert_with(ptr, val, fill);
  }

  /// Instrumentation to become irrevocable... This is not allowed in PTM
  void becomeIrrevocable() { std::terminate(); }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }

private:
  /// Check if the given address is on the thread's stack, and hence does not
  /// need instrumentation.  We also check for captured memory
  bool accessDirectly(void *ptr) {
    if (allocator.checkCaptured(ptr))
      return true;
    return frame.onStack(ptr);
  }
};