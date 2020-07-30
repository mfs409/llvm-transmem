#pragma once

#include <cstdlib>

#include "../common/deferred.h"

/// CGL is an STM algorithm with the following characteristics:
/// - All transactions are protected by a single coarse-grained lock, and thus
///   there is no concurrency among transactions.
/// - All writes are performed immediately.
///
/// CGL can be customized in the following ways:
/// - Lock implementation (spin lock, std::mutex, etc)
///
/// Note that while we template on the following, the options are limited by CGL
/// not taking instrumented code paths:
/// - Stack Frame (only used to manage transaction nesting)
/// - Allocator (to standardize malloc/free behavior across implementations)
template <class LOCK, class STACKFRAME, class ALLOCATOR> class CGL {
  /// Globals is a wrapper around all of the global variables used by CGL
  struct Globals {
    /// The lock used for all concurrency control.
    LOCK lock;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// Return the irrevocability state of the thread
  bool isIrrevoc() { return true; }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *) {}

  /// construct a thread's transaction context by zeroing its nesting depth
  CGL() {}

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx() {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      globals.lock.acquire();
      // Start logging allocations
      allocator.onBegin();
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      globals.lock.release();
      allocator.onCommit();
      deferredActions.onCommit();
      frame.onCommit();
    }
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  void *txAlloc(size_t size) {
    return allocator.alloc(size, [&]() {});
  }

  /// To allocate aligned memory, we must also log it, so we can reclaim it if
  /// the transaction aborts
  void *txAAlloc(size_t A, size_t size) {
    return allocator.alignAlloc(A, size, [&]() {});
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void txFree(void *addr) { allocator.reclaim(addr); }

  /// Transactional read:
  template <typename T> T read(T *ptr) { return *ptr; }

  /// Transactional write
  template <typename T> void write(T *ptr, T val) { *ptr = val; }

  /// Instrumentation to become irrevocable... we're already irrevocable
  void becomeIrrevocable() { return; }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }
};