#pragma once

#include <cstdlib>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/p_status_t.h"

/// P_CGL_Eager is an PTM algorithm with the following characteristics:
/// - All transactions are protected by a single coarse-grained lock, and thus
///   There is no concurrency among transactions.
/// - All writes are performed immediately.
///
/// P_CGL_Eager can be customized in the following ways:
/// - Lock implementation (spin lock, std::mutex, etc)
/// - The undo log implementation
/// - Stack Frame (only used to manage transaction nesting)
/// - Allocator (to standardize malloc/free behavior across implementations)
template <class LOCK, class UNDOLOG, class STACKFRAME, class ALLOCATOR>
class P_CGL_Eager {
  /// Globals is a wrapper around all of the global variables used by CGL
  struct Globals {
    /// The lock used for all concurrency control.
    LOCK lock;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// The transaction's persistent status
  P_status p_status;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// The transaction's undo log, in case of crashes
  UNDOLOG p_undolog;

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
  P_CGL_Eager() : p_status(nullptr, &p_undolog, &allocator) {}

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
      if (p_undolog.size() == 0) {
        // possibly flush writes to captured memory
        if (allocator.p_precommit()) {
          pmem_sfence();
        }
        allocator.commitMallocs();
        globals.lock.release();
        allocator.commitFrees();
      deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // Writer transaction
      allocator.p_precommit();
      // be sure all writes/clwbs to main memory order before status change
      pmem_sfence();
      allocator.commitMallocs();
      p_status.on_eager_commit();
      globals.lock.release();
      p_undolog.p_clear();
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
  template <typename T> T read(T *ptr) { return *ptr; }

  /// Transactional write
  template <typename T> void write(T *ptr, T val) {
    // No instrumentation if on stack or captured
    if (accessDirectly(ptr)) {
      *ptr = val;
      allocator.p_flushOnCapturedStore(ptr);
      return;
    }

    // On first write, we need to mark self as having a valid undo log
    if (p_undolog.empty()) {
      p_status.on_eager_start();
    }

    // log it (includes clwb and fence)
    p_undolog.p_push_back(ptr);

    // update it and clwb it
    *ptr = val;
    pmem_clwb(ptr);
  }

  /// Instrumentation to become irrevocable... This is not allowed in PTM
  void becomeIrrevocable() { /*std::terminate();*/ }

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
