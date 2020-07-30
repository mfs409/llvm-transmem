#pragma once

#include <atomic>
#include <setjmp.h>

#include "../common/deferred.h"
#include "../common/pad_word.h"
#include "../common/platform.h"
#include "../common/vlog_t.h"

/// NOrec is an STM algorithm with the following characteristics:
/// - Uses values for validation, instead of a lock table or other metadata
/// - Performs speculative writes out of place (uses redo)
///
/// NOrec can be customized in the following ways:
/// - Redo Log (data structure and granularity of chunks)
/// - Value Log (primarily for atomicity experiments)
/// - EpochManager (quiescence and irrevocability)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to become irrevocable on too many allocations)
///
/// Note that it is possible to produce an incorrect STM.  For example, no
/// irrevocability + irrevocable CM can lead to transactions spinning forever.
/// Similarly, note that NOrec does not require Quiescence for privatization
/// safety, but does require Quiescence if it uses an allocator that can return
/// memory to the operating system.
template <class REDOLOG, class VALUELOG, class EPOCH, class CM,
          class STACKFRAME, class ALLOCATOR>
class NOrec {
  /// Globals is a wrapper around all of the global variables used by NOrec
  struct Globals {
    /// The lock used for all concurrency control.
    pad_dword_t lock;

    /// The contention management metadata
    typename CM::Globals cm;

    /// Quiescence support
    typename EPOCH::Globals epoch;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// Checkpoint lets us reset registers and the instruction pointer in the
  /// event of an abort
  jmp_buf *checkpoint;

  /// For managing thread IDs, Quiescence, and Irrevocability
  EPOCH epoch;

  /// Contention manager
  CM cm;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The value of global_lock when this transaction started
  uint64_t lock_snapshot;

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
  /// Return the irrevocability state of the thread
  bool isIrrevoc() { return epoch.isIrrevoc(); }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) { frame.setBottom(addr); }

  /// construct a thread's transaction context by zeroing its nesting depth and
  /// giving it an ID.
  NOrec() : epoch(globals.epoch), cm() {}

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx(jmp_buf *b) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // Save the checkpoint and set the stack bottom
      checkpoint = b;
      frame.setBottom(b);

      // Start logging allocations
      allocator.onBegin();

      // Get the start time, and put it into the epoch.  epoch.onBegin will wait
      // until there are no irrevocable transactions.
      //
      // NB: rather than block if transaction is starting during a writeback, we
      //     just rewind our start time to even
      lock_snapshot = globals.lock.val;
      if (lock_snapshot & 1)
        lock_snapshot--;
      epoch.onBegin(globals.epoch, lock_snapshot);

      // Notify CM of intention to start.  If return true, become irrevocable
      if (cm.beforeBegin(globals.cm)) {
        becomeIrrevocable();
      }
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      // Irrevocable commit is easy, because we reset the lists when we became
      // irrevocable
      if (epoch.isIrrevoc()) {
        epoch.onCommitIrrevoc(globals.epoch);
        cm.afterCommit(globals.cm);
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }
      // fast-path for read-only transactions must still quiesce before freeing
      if (redolog.isEmpty()) {
        epoch.clearEpoch(globals.epoch);
        valuelog.clear();
        cm.afterCommit(globals.cm);
        epoch.quiesce(globals.epoch, lock_snapshot);
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }
      // Commit a writer transaction:

      // Try to grab lock... if we can't, then validate and try again
      uint64_t from = lock_snapshot;
      while (!globals.lock.val.compare_exchange_strong(from, from + 1)) {
        from = validate();
      }
      lock_snapshot = from;

      // replay redo log and write back
      redolog.writeback_atomic();

      // depart epoch table (fence) and then release the lock
      epoch.clearEpoch(globals.epoch);
      globals.lock.val = 2 + lock_snapshot;

      // clear lists.  Quiesce before freeing, noting that we need threads to be
      // > lock_snapshot
      redolog.reset();
      valuelog.clear();
      cm.afterCommit(globals.cm);
      epoch.quiesce(globals.epoch, lock_snapshot + 1);
      allocator.onCommit();
      deferredActions.onCommit();
      frame.onCommit();
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

    // We need to get at least some of the bytes from main memory, so use the
    // consistent read algorithm
    T from_mem = REDOLOG::perform_transactional_read(ptr);
    while (lock_snapshot != globals.lock.val) {
      lock_snapshot = validate();
      from_mem = REDOLOG::perform_transactional_read(ptr);
    }

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

  /// Instrumentation to become irrevocable... This is essentially an early
  /// commit
  void becomeIrrevocable() {
    // Immediately return if we are already irrevocable
    if (epoch.isIrrevoc()) {
      return;
    }

    // try_irrevoc will return true only if we got the token and quiesced
    if (!epoch.tryIrrevoc(globals.epoch)) {
      abortTx();
    }

    // Quick validation, since we know the lock is even and immutable
    if (!valuelog.validate_nonatomic()) {
      epoch.onCommitIrrevoc(globals.epoch);
      abortTx();
    }

    // replay redo log
    redolog.writeback_nonatomic();

    // clear lists
    allocator.onCommit();
    valuelog.clear();
    redolog.reset();
  }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }

private:
  /// Validation.  We need to make sure the lock is even and does not change
  /// while we check values
  uint64_t validate() {
    while (true) {
      uint64_t time = globals.lock.val.load();
      // spinning if writer currently holding the lock
      if (time & 1) {
        continue;
      }
      bool valid = valuelog.validate_atomic();
      if (!valid) {
        abortTx();
      }
      // NB: can't update epoch until after validation, since concurrent free()
      //     could be to location being validated
      epoch.setEpoch(globals.epoch, time);
      if (time == globals.lock.val)
        return time;
    }
  }

  /// Abort the transaction
  void abortTx() {
    // We can exit the Epoch right away, so that other threads don't have to
    // wait on this thread.
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // reset all lists
    redolog.reset();
    valuelog.clear();
    allocator.onAbort(); // this reclaims all mallocs
    deferredActions.onAbort();
    frame.onAbort();
    longjmp(*checkpoint, 1); // Takes us back to calling beginTx()
  }

  /// Check if the given address is on the thread's stack, and hence does not
  /// need instrumentation.  Note that if the thread is irrevocable, we also say
  /// that instrumentation is not needed
  bool accessDirectly(void *ptr) {
    if (epoch.isIrrevoc())
      return true;
    if (allocator.checkCaptured(ptr))
      return true;
    return frame.onStack(ptr);
  }
};
