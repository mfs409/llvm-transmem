#pragma once

#include <cassert>
#include <exception>

#include "../common/deferred.h"
#include "../common/pad_word.h"
#include "../common/platform.h"
#include "../common/vlog_t.h"

/// Cohorts is an STM algorithm with the following characteristics:
/// - Uses values for validation, instead of a lock table or other metadata
/// - Performs speculative writes out of place (uses redo)
/// - Commits transactions in batches, so that no aborts are possible except at
///   commit time.  This results in fence-less execution, even on CPUs with
///   relaxed memory consistency.
///
/// Cohorts can be customized in the following ways:
/// - Redo Log (data structure and granularity of chunks)
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to employ captured memory optimizations)
///
/// NB: Cohorts does not support irrevocability.  Also, note that the published
///     cohorts algorithm has a "turbo mode", wherein a the last thread in a
///     sealed cohort skips all instrumentation for the remainder of its
///     transaction, and then commits first.  We do not yet support this.
template <class REDOLOG, class VALUELOG, class STACKFRAME, class ALLOCATOR>
class Cohorts {
  /// Globals is a wrapper around all of the global variables used by Cohorts
  struct Globals {
    /// Counter for started transactions
    pad_dword_t STARTED;

    /// Counter for ready-to-commit transactions
    pad_dword_t SEALED;

    /// Counter for finished transactions
    pad_dword_t FINISHED;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// For managing the stack frame
  STACKFRAME frame;

  /// a cache of the cohort state when we began
  uint64_t start_time;

  // a redolog, since this is a lazy TM
  REDOLOG redolog;

  /// A read set for this transaction
  VALUELOG valuelog;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// A thread is never irrevocable in Cohorts...
  bool isIrrevoc() { return false; }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) { frame.setBottom(addr); }

  /// construct a thread's transaction context
  Cohorts() {}

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx(void *b) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // Set the stack bottom
      frame.setBottom(b);

      // Start logging allocations
      allocator.onBegin();

      // Try to enter the current cohort (or create it)
      while (true) {
        globals.STARTED.val++;
        // If cohort already sealed, wait...
        if (globals.SEALED.val == globals.FINISHED.val)
          break;
        globals.STARTED.val--;
        while (globals.SEALED.val != globals.FINISHED.val) {
        }
      }
      start_time = globals.FINISHED.val;
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  bool commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      // read-only fast path
      if (redolog.isEmpty()) {
        globals.STARTED.val--;
        valuelog.clear();
        // NB: there is some magic as to why it is safe to free without
        //     quiescing for read-only transactions.  The argument goes like
        //     this: if this transaction can safely remove the data, then it
        //     could have run before all concurrent transactions started, and
        //     still free the data.  Thus the data cannot be concurrently
        //     accessed by other transactions, because there is no way that they
        //     can have a reference and be doomed to abort on account of this
        //     transaction (because this transaction is read-only).  The same
        //     logic means that post-commit free() isn't a problem.
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        return true;
      }

      // Get commit order... This also seals the cohort
      uintptr_t my_order = globals.SEALED.val++;
      // Wait for all started transactions to reach this point...
      globals.STARTED.val--;
      while (globals.STARTED.val) {
      }
      // knowing the sealed value will help us to quiesce later...
      uintptr_t seal_snap = globals.SEALED.val;
      // Wait for turn to commit
      while (globals.FINISHED.val < my_order) {
      }

      // only writeback if validation succeeds
      bool ret = false;

      // No need to validate if first in cohort...
      if ((my_order == start_time) || valuelog.validate_fastexit_nonatomic()) {
        redolog.writeback_atomic();
        ret = true;
      }

      // Let the next person commit
      globals.FINISHED.val++;

      // clear logs
      valuelog.clear();
      redolog.reset();

      // Since we are using values for validation, we need some kind of
      // lightweight quiescence before freeing. Since SEALED and FINISHED are
      // monotonically increasing, we know that for as long as FINISHED <
      // SEALED, someone could be validating a value we're about to free.  But
      // we can't just use SEALED... seal_snap from before is the right value to
      // use.
      while (globals.FINISHED.val < seal_snap) {
      }

      // If this transaction succeeded, clear mallocs, do frees
      if (ret) {
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
      }
      // else clear frees, reclaim mallocs
      else {
        allocator.onAbort();
        deferredActions.onAbort();
        frame.onAbort();
      }
      return ret;
    }
    // nested
    else {
      return true;
    }
  }

  /// To allocate memory, we must also log it, so we can reclaim it if the
  /// transaction aborts
  void *txAlloc(size_t size) {
    return allocator.alloc(size, [&]() { becomeIrrevocable(); });
  }

  /// To allocate aligned memory, we must also log it, so we can reclaim it if
  /// the transaction aborts
  void *txAAlloc(size_t A, size_t size) {
    return allocator.alignAlloc(A, size, [&]() { becomeIrrevocable(); });
  }

  /// To free memory, we simply wait until the transaction has committed, and
  /// then we free.
  void txFree(void *addr) { allocator.reclaim(addr); }

  /// Transactional read:
  template <typename T> T read(T *ptr) {
    // If we can just do the read, then just do it
    if (accessDirectly(ptr)) {
      return *ptr;
    }

    // Lookup in redo log to populate ret.  Note that prior casting can lead to
    // ret having only some bytes properly set
    T ret;
    int found_mask = redolog.find(ptr, ret);
    // If we found all the bytes in the redo log, then it's easy
    int desired_mask = (1UL << sizeof(T)) - 1;
    if (desired_mask == found_mask) {
      return ret;
    }

    // We need to get at least some of the bytes from main memory
    T from_mem = *ptr;

    // log the value to the valuelog
    vlog_t u;
    u.make(ptr, from_mem);
    valuelog.push_back(u);

    // If redolog was a partial hit, reconstruction is needed
    if (!found_mask) {
      return from_mem;
    }
    REDOLOG::reconstruct(from_mem, ret, found_mask);
    return ret;
  }

  /// Transactional write
  template <typename T> void write(T *ptr, T val) {
    if (accessDirectly(ptr)) {
      *ptr = val;
    } else {
      redolog.insert(ptr, val);
    }
  }

  /// Irrevocability is not supported
  void becomeIrrevocable() { std::terminate(); }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }

private:
  /// Check if the given address is on the thread's stack or was allocated by
  /// the thread, and hence does not need instrumentation
  bool accessDirectly(void *ptr) {
    if (allocator.checkCaptured(ptr))
      return true;
    return frame.onStack(ptr);
  }
};
