#pragma once

#include <atomic>
#include <setjmp.h>
#include <x86intrin.h>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/pad_word.h"
#include "../common/platform.h"
#include "../common/vlog_t.h"

/// HybridNOrecTwoCounter is a Hybrid TM algorithm with the following
/// characteristics:
/// - It uses two counters for concurrency control: one is for HTM to notify STM
///   of commit, and one is for STM to notify STM of commit.
/// - HTM cannot commit during STM writeback
/// - Uses values for validation, instead of a lock table or other metadata
/// - Performs speculative writes out of place (uses redo)
///
/// HybridNOrecTwoCounter can be customized in the following ways:
/// - RETRY (HTM attempts that fail before falling back to STM)
/// - Redo Log (data structure and granularity of chunks)
/// - EpochManager (quiescence and irrevocability)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to become irrevocable on too many allocations)
template <int RETRY, class REDOLOG, class EPOCH, class CM, class STACKFRAME,
          class ALLOCATOR>
class HybridNOrecTwoCounter {
  /// Globals is a wrapper around all of the global variables used by NOrec
  struct Globals {
    /// Padding to ensure lock is not falsely shared
    pad_dword_t gap0[11];

    /// The lock used for all concurrency control.
    pad_dword_t lock;

    /// More padding
    pad_dword_t gap1[10];

    /// The counter used for HTM commit
    pad_dword_t htm_counter;

    /// More padding
    pad_dword_t gap2[9];

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

  /// The timestamp to remember when the last HTM committed
  uint64_t htm_snapshot;

  // a redolog, since this is a lazy TM
  REDOLOG redolog;

  /// A read set for this transaction
  MiniVector<vlog_t> valuelog;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// Indicate if the transaction is running in HTM mode or STM mode
  bool htm_path;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// Return the irrevocability state of the thread
  bool isIrrevoc() { return epoch.isIrrevoc(); }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) { frame.setBottom(addr); }

  /// construct a thread's transaction context
  HybridNOrecTwoCounter() : epoch(globals.epoch), cm(), htm_path(false) {}

  /// HTM path, without instrumentation
  bool beginHTM(void) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // notify CM, in case we're supposed to wait, but ignore any suggestion to
      // become irrevocable
      cm.beforeBegin(globals.cm);

      // Record how many times HTM has aborted
      int retry_counter = 0;
      uint64_t valid_lock_val, status;
    retry:
      // If lock is held for STM write back, or irrevocable STM is running, then
      // we can't start yet.  This sort of spinning should not cost our HTM
      // attempts.
      do {
        valid_lock_val = globals.lock.val.load(std::memory_order_relaxed);
      } while (valid_lock_val & 1 || epoch.existIrrevoc(globals.epoch));
      // start hardware transaction
      status = _xbegin();
      if (status == _XBEGIN_STARTED) {
        // Abort if concurrent STM is committing
        // NB: also subscribes so tx will abort if concurrent STM ever commits
        if (globals.lock.val & 1 || epoch.existIrrevoc(globals.epoch)) {
          _xabort(66);
        }
        htm_path = true;
        // NB: epoch, and allocator *do not know about this transaction*
        return true;
      } else if ((status & _XABORT_EXPLICIT) && _XABORT_CODE(status) == 66) {
        // We aborted during start, due to committing STM, so try again
        retry_counter = 0;
        goto retry;
      } else if (!(status & _XABORT_CAPACITY) && retry_counter <= RETRY) {
        // Abort reason is NOT capacity, and we have more retries
        retry_counter++;
        goto retry;
      } else {
        // Capacity abort or too many attempts... switch to STM mode
        frame.onEnd();
        htm_path = false;
        // depart CM, so that we can re-enter it from STM path
        cm.afterAbort(globals.cm, epoch.id);
        return false;
      }
    }
    // if we're not nested, take the appropriate path for the state we're in...
    return htm_path;
  }

  /// Instrumentation to run at the beginning of an STM transaction
  void beginSTM(jmp_buf *b) {
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
        --lock_snapshot;
      // Read the HTM counter
      htm_snapshot = globals.htm_counter.val;
      // NB: only STM transactions participate in quiescence...
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
      // To commit from HTM path, just notify STM to validate, then commit
      if (htm_path) {
        globals.htm_counter.val++;
        _xend();
        // NB: There is *significant* complexity to the question of whether we
        //     ought to quiesce here or not.  The specific problem is that an
        //     HTM transaction might have called free(), and if it did, then
        //     that memory might go back to the OS, in which case a concurrent
        //     software transaction could take a segfault during validation
        //     (because of value-based validation).
        //
        //     It's not so easy, though: if the HTM transaction's free()
        //     actually returned memory to the OS, that would be a syscall, and
        //     the HTM transaction would have aborted.  So the HTM transaction
        //     must have freed something, and then some other thread must have
        //     freed something, and that second free must have led to a full
        //     page becoming free (a page that contained both of the regions
        //     that were freed).  Note that if the second freeing thread is STM,
        //     then it will quiesce, so we don't have anything to worry about.
        //     But if the second freeing thread is non-transactional, we have a
        //     problem.
        //
        //     A second possible problem is that the free() could be followed by
        //     a malloc() from a nontransactional thread, whose writes to the
        //     memory could then race with validation in a software transaction.
        //
        //     If we wanted to address the problems, we'd need HTM to take an
        //     instrumented path that deferred all frees(), so that we could
        //     xend, quiesce, and then free().  We are instead going to ignore
        //     the problem, because (a) it is likely that anyone using this TM
        //     is also using something like jemalloc, which never gives memory
        //     back to the OS, and (b) these problems don't manifest in any of
        //     our benchmarks, and that's good enough for a research artifact.
        cm.afterCommit(globals.cm);
        deferredActions.onCommit();
        frame.onCommit();
        // NB: Recall that HTM is not visible to epoch or allocator
        return;
      }
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
        // NB: validate() will update htm_snapshot
        from = validate();
      }

      // Since we have acquired global_lock, no new HTM can commit.
      // However, it's possible that an HTM committed since our last
      // validation, so we need to validate one more time.
      if (htm_snapshot !=
          globals.htm_counter.val.load(std::memory_order_relaxed)) {
        bool valid = true;
        for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
          valid = valid && it->checkValue();
        }
        if (!valid) {
          // Release the lock before abort
          globals.lock.val = from;
          abortTx();
        }
      }

      lock_snapshot = from;

      // replay redo log
      redolog.writeback();

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
  ///
  /// NB: we assume that a data type is never aligned such that it crosses a
  /// 64-byte boundary, since that is the granularity of redo log entries.
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
    T from_mem = *ptr;

    // We need to check both HTM and STM timestamp
    while (lock_snapshot != globals.lock.val ||
           htm_snapshot !=
               globals.htm_counter.val.load(std::memory_order_relaxed)) {
      // NB: validate() will update htm_snapshot
      lock_snapshot = validate();
      from_mem = *ptr;
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

  /// Instrumentation to become irrevocable...
  void becomeIrrevocable() {
    // Immediately return if we are already irrevocable
    if (epoch.isIrrevoc()) {
      return;
    }

    // NB: becomeIrrevocable() is only reachable from instrumented code, and in
    //     HybridNOrecTwoCounter, HTM threads never use the instrumented code
    //     path.  Thus HTM will never request irrevocability in-flight.  HTM may
    //     require irrevocability, e.g., due to a syscall, but if that happens,
    //     the HTM will abort and switch to the software path.

    // try_irrevoc will return true only if we got the token and quiesced
    if (!epoch.tryIrrevoc(globals.epoch)) {
      abortTx();
    }

    // At this point, no new transactions can start, and all STM transactions
    // are done.  However, we do not know that all HTM transactions are done.
    // Rather than make all HTM transactions participate in Epochs, we will
    // explicitly shoot down all HTM transactions now, by bumping the global
    // lock to which they subscribed:
    globals.lock.val += 2;

    // Quick validation, since we know the lock is even and immutable
    for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
      if (!it->checkValue()) {
        epoch.onCommitIrrevoc(globals.epoch);
        abortTx();
      }
    }

    // replay redo log
    redolog.writeback();

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
      // spin if writer currently holds the lock
      if (time & 1) {
        continue;
      }

      // read from HTM counter first
      htm_snapshot = globals.htm_counter.val.load(std::memory_order_relaxed);

      bool valid = true;
      for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
        valid = valid && it->checkValue();
      }
      if (!valid) {
        abortTx();
      }
      // NB: can't update epoch until after validation, since concurrent free()
      //     could be to location being validated
      epoch.setEpoch(globals.epoch, time);
      if (time == globals.lock.val &&
          htm_snapshot ==
              globals.htm_counter.val.load(std::memory_order_relaxed)) {
        return time;
      }
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