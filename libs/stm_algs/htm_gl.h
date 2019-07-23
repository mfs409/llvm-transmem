#pragma once

#include <cstdlib>
#include <x86intrin.h>

#include "../common/deferred.h"
#include "../common/pad_word.h"

/// HTM_GL uses TSX with a fallback to a single global spinlock.
///
/// HTM_GL can be customized in the following ways:
/// - Number of HTM aborts before falling back to the global lock.
///
/// Note that while we template on the following, the options are limited by
/// HTM_GL not taking instrumented code paths:
/// - Stack Frame (only used to manage transaction nesting)
/// - Allocator (to standardize malloc/free behavior across implementations)
template <class STACKFRAME, class ALLOCATOR, int RETRIES> class HTM_GL {
  /// Globals is a wrapper around all of the global variables used by HTM_GL
  struct Globals {
    /// The lock used for fallback path when HTM could not commit
    pad_dword_t gl_lock;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// For managing the stack frame
  STACKFRAME frame;

  /// It specifies if execution path is HTM
  bool htm_path;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// Return the irrevocability state of the thread
  bool isIrrevoc() { return true; }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) {}

  /// construct a thread's transaction context by zeroing its nesting depth
  HTM_GL() : htm_path(false) {}

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx() {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // we need to now how many times the HTM aborted
      int retry_counter = 0;
      uint64_t valid_lock_val, status;
      // On transaction abort, restart from here
    retry:
      // If lock is held, wait until it is released
      do {
        valid_lock_val = globals.gl_lock.val.load(std::memory_order_relaxed);
      } while (valid_lock_val & 1);
      // Start hardware transaction
      status = _xbegin();
      if (status == _XBEGIN_STARTED) {
        // check the lock is not held by other threads
        // also, this prevent lock status changing during htm execution
        if (globals.gl_lock.val.load(std::memory_order_relaxed) & 1)
          _xabort(66);
        htm_path = true;
        // Start logging allocations.  Note that this TM probably uses an
        // immediate allocation manager, since it is unlikely to take the
        // instrumented path
        allocator.onBegin();
        return; // HTM succeed!
      } else if ((status & _XABORT_EXPLICIT) && _XABORT_CODE(status) == 66) {
        // global lock is on, so we go back to
        // the beginning of transaction and reset the retry counter
        retry_counter = 0;
        goto retry;
      } else if (!(status & _XABORT_CAPACITY) && retry_counter <= RETRIES) {
        // abort reason is not lack of capacity, retry times less than the
        // threshold
        retry_counter++;
        goto retry;
      } else {
        // slow path, global lock
        while (true) {
          valid_lock_val = globals.gl_lock.val.load(std::memory_order_relaxed);
          // acquire the global lock
          if (!(valid_lock_val & 1)) {
            if (globals.gl_lock.val.compare_exchange_strong(valid_lock_val, 1))
              break;
          }
        }
        htm_path = false;
        // Start logging allocations
        allocator.onBegin();
        return; // global lock grabbed
      }
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      if (htm_path) {
        _xend();
      } else {
        globals.gl_lock.val.store(0);
      }
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