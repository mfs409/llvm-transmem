#pragma once

#include <atomic>
#include <setjmp.h>
#include <x86intrin.h>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/pad_word.h"
#include "../common/platform.h"
#include "../common/vlog_t.h"

/// ReducedHardwareNOrec is a Hybrid TM algorithm based on the prefix and
/// postfix HTM from Matveev ASPLOS 2015.  It has the following characteristics:
/// - It contains three execution paths: 1st HTM, 2nd PrefixHTM + PostfixHTM,
///   3rd STM
/// - The prefix/postfix transition happens on first write, and HTM cannot
///   commit during STM writeback
/// - The STM fallback, and validation in general, is based on NOrec
/// - The STM path performs speculative writes out of place (uses redo)
///
/// ReducedHardwareNOrec can be customized in the following ways:
/// - HTM attempts that fail before falling back to prefix/postfix
/// - Postfix attempts that fail before falling back to STM
/// - Redo Log (data structure and granularity of chunks)
/// - EpochManager (quiescence and irrevocability)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to become irrevocable on too many allocations)
template <int HTM_RETRY, int POSTFIX_RETRY, class REDOLOG, class EPOCH,
          class CM, class STACKFRAME, class ALLOCATOR>
class ReducedHardwareNOrec {
  /// Globals is a wrapper around all of the global variables used by NOrec
  struct Globals {
    /// Padding, to ensure lock is not going to suffer from false sharing
    pad_dword_t gap0[11];

    /// The lock used for all concurrency control.
    pad_dword_t lock;

    /// Place significant distance between lock and htm_counter, but without
    /// hurting associativity
    pad_dword_t gap1[10];

    /// The counter used for HTM commit
    pad_dword_t htm_counter;

    /// More padding, to keep htm_counter from suffering false sharing
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

  /// 1st path ---> HTM
  /// Indicate if the transaction is running in HTM mode or STM mode
  bool htm_path;

  /// 2nd path ---> Prefix HTM + Postfix HTM
  /// Indicate if the prefix HTM is active
  bool is_rh_prefix_active;

  /// Indicate if the postfix HTM is active
  bool is_rh_postfix_active;

  /// Count postfix aborts
  int postfix_aborts;

  /// 3nd path ---> NOrec STM
  /// Indicate if the STM path is active
  bool stm_path;

  /// deferredActions manages all functions that should run after transaction
  /// commit.
  DeferredActionHandler deferredActions;

public:
  /// Return the irrevocability state of the thread
  bool isIrrevoc() { return epoch.isIrrevoc(); }

  /// Set the current bottom of the transactional part of the stack
  void adjustStackBottom(void *addr) { frame.setBottom(addr); }

  /// construct a thread's transaction context
  ReducedHardwareNOrec()
      : epoch(globals.epoch), cm(), htm_path(false), is_rh_prefix_active(false),
        is_rh_postfix_active(false), postfix_aborts(0), stm_path(false) {}

  /// HTM path, without instrumentation
  bool beginHTM(void) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // notify CM, in case we're supposed to wait, but ignore any suggestion to
      // become irrevocable
      cm.beforeBegin(globals.cm);

      // Record how many times HTM has aborted
      int retry_counter = 0;
      uint64_t valid_lock_val;
    retry:
      // If lock is held for STM write back, or irrevocable STM is running, then
      // we can't start yet.  This sort of spinning should not cost our HTM
      // attempts.
      do {
        valid_lock_val = globals.lock.val.load(std::memory_order_relaxed);
      } while (valid_lock_val & 1 || epoch.existIrrevoc(globals.epoch));
      // start hardware transaction
      uint64_t status = _xbegin();
      if (status == _XBEGIN_STARTED) {
        // Abort if concurrent STM is committing
        // NB: also subscribes so tx will abort if concurrent STM ever commits
        if (globals.lock.val & 1 || epoch.existIrrevoc(globals.epoch)) {
          _xabort(66);
        }
        htm_path = true;
        // NB: cm, epoch, and allocator *do not know about this transaction*
        return true;
      } else if ((status & _XABORT_EXPLICIT) && _XABORT_CODE(status) == 66) {
        // We aborted during start, due to committing STM, so try again
        retry_counter = 0;
        goto retry;
      } else if (!(status & _XABORT_CAPACITY) && retry_counter <= HTM_RETRY) {
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
    // nesting: return htm_path to direct to instrumented or uninstrumented code
    return htm_path;
  }

private:
  /// In the begin() code, we call this to launch an HTM "prefix" transaction.
  /// A "prefix" transaction is one that uses HTM to avoid any read set logging,
  /// and which switches to non-HTM execution upon first write.
  ///
  /// It should *not* be the common case that a prefix transaction reaches the
  /// commit() function, because that would imply that the transaction could
  /// commit using only HTM, and if so, it should have been able to do so as a
  /// pure HTM transaction.  Thus we may assume that transactions calling this
  /// will become postfix transactions, and hence we will do things like enter
  /// the epoch.
  bool start_rh_htm_prefix() {
    // NB: we already saved the checkpoint, entered the frame, and entered the
    // allocator

    // notify CM, in case we're supposed to wait, but ignore any suggestion to
    // become irrevocable
    cm.beforeBegin(globals.cm);

    // Record how many times HTM has aborted
    int retry_counter = 0;

  retry:
    // If lock is held for STM write back, or irrevocable STM is running
    // wait until it is released
    do {
      lock_snapshot = globals.lock.val.load(std::memory_order_relaxed);
    } while (lock_snapshot & 1 || epoch.existIrrevoc(globals.epoch));

    // enter the epoch *before* xbegin, so that we don't track it
    epoch.onBegin(globals.epoch, lock_snapshot);

    // start hardware transaction
    uint64_t status = _xbegin();

    if (status == _XBEGIN_STARTED) {
      // Abort if concurrent STM is committing (and subscribe to global lock)
      if (globals.lock.val != lock_snapshot) {
        _xabort(66);
      }
      is_rh_prefix_active = true;
      return true;
    } else if ((status & _XABORT_EXPLICIT) && _XABORT_CODE(status) == 66) {
      // We aborted during start, due to committing STM, so try again
      retry_counter = 0;
      epoch.clearEpoch(globals.epoch);
      goto retry;
    } else if (!(status & _XABORT_CAPACITY) && retry_counter <= HTM_RETRY) {
      // Abort reason is NOT capacity, and we have more retries
      // NB: HTM_RETRY is correct here, not POSTFIX_RETRY
      ++retry_counter;
      epoch.clearEpoch(globals.epoch);
      goto retry;
    }

    // Capacity abort or too many attempts... switch to STM mode
    is_rh_prefix_active = false;
    stm_path = true;
    // depart CM, so that we can re-enter it from STM path
    cm.afterAbort(globals.cm, epoch.id);
    // depart epoch, so we can re-enter it from STM path
    epoch.clearEpoch(globals.epoch);
    return false;
  }

public:
  /// Instrumentation to run at the beginning of an STM or prefix transaction
  void beginSTM(jmp_buf *b) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // Save the checkpoint and set the stack bottom
      checkpoint = b;
      frame.setBottom(b);

      // Start logging allocations
      allocator.onBegin();

      // Try 2nd path if we haven't already aborted in 3rd path...
      if (!stm_path && start_rh_htm_prefix()) {
        return; // we're an HTM prefix transaction
      }

      // Get the start time, and put it into the epoch.  epoch.onBegin will wait
      // until there are no irrevocable transactions.
      //
      // NB: rather than block if transaction is starting during a writeback, we
      //     just rewind our start time to even
      lock_snapshot = globals.lock.val;
      if (lock_snapshot & 1)
        lock_snapshot--;
      // Read the HTM counter
      htm_snapshot = globals.htm_counter.val;
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
        //     But if the second freeing thread is non-transactions, we have a
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

      // Commit a read-only prefix htm (unlikely) or a postfix htm
      if (is_rh_prefix_active || is_rh_postfix_active) {
        if (is_rh_postfix_active) {
          globals.htm_counter.val++; // only for writers...
        }
        _xend();
        is_rh_prefix_active = false;
        is_rh_postfix_active = false;
        postfix_aborts = 0;
        // clear the allocator and cm, then quiesce and free
        epoch.clearEpoch(globals.epoch);
        cm.afterCommit(globals.cm);
        epoch.quiesce(globals.epoch, lock_snapshot);
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // reset the stm path
      stm_path = false;

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
        // update the HTM counter before validate ...
        htm_snapshot = globals.htm_counter.val.load(std::memory_order_relaxed);
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

      // replay redo log and write back
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
  template <typename T> T read(T *ptr) {
    // handle the path in prefix HTM (no need to check htm flag, since it's not
    // instrumented)
    if (is_rh_prefix_active || is_rh_postfix_active || accessDirectly(ptr)) {
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
    // on first prefix write, switch modes
    if (is_rh_prefix_active) {
      start_rh_htm_postfix();
    }
    // Again, no need to check for htm mode, since htm is uninstrumented
    if (is_rh_postfix_active || accessDirectly(ptr)) {
      *ptr = val;
    } else {
      redolog.insert(ptr, val);
    }
  }

private:
  /// On first write from a prefix transaction, we must switch to being a
  /// postfix transaction
  void start_rh_htm_postfix() {
    // we start by committing the prefix transaction.  Note that by definition
    // it has no shared writes.  However, it may have populated the allocator,
    // and it is in the epoch and CM.  This means, e.g., no irrevocable
    // transaction can start during this function's execution

    // Recall that the prefix transaction could handle concurrent HTM commits,
    // but could not handle concurrent STM commits.  The postfix must not
    // continue if any STM commits that would have caused the prefix to abort.
    // It also must not continue if any future HTM commits.  We can use
    // globals.lock.val (read previously) for the first condition.  But we must
    // read the htm_counter *right now*, before xend.
    htm_snapshot = globals.htm_counter.val;
    _xend();

    // immediately start the prefix
    is_rh_prefix_active = false;
    is_rh_postfix_active = true;
    int retry_counter = 0;

  retry:
    uint64_t status = _xbegin();
    // if we started, and memory hasn't changed, we're postfix
    if (status == _XBEGIN_STARTED) {
      // abort if memory has changed in un-validatable ways
      if (globals.lock.val != lock_snapshot ||
          globals.htm_counter.val != htm_snapshot) {
        _xabort(66);
      }
      return;
    }
    // If we aborted, but it's not for capacity, memory hasn't changed, and we
    // have more retries, keep trying...
    else if (!(status & _XABORT_CAPACITY) && retry_counter <= HTM_RETRY) {
      ++retry_counter;
      goto retry;
    }
    // If we aborted because memory changed, we may want to try prefix again
    else if ((status & _XABORT_EXPLICIT) && postfix_aborts <= POSTFIX_RETRY) {
      ++postfix_aborts;
    }
    // Otherwise we will switch to STM
    else {
      postfix_aborts = 0;
      stm_path = true;
    }
    // exit the epoch, exit cm, and clean up the allocator
    is_rh_postfix_active = false;
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);
    allocator.onAbort();
    deferredActions.onAbort();
    frame.onAbort();

    // return to calling beginSTM():
    longjmp(*checkpoint, 1);
  }

public:
  /// Instrumentation to become irrevocable... This is essentially an early
  /// commit
  void becomeIrrevocable() {
    // Immediately return if we are already irrevocable
    if (epoch.isIrrevoc()) {
      return;
    }

    // NB: becomeIrrevocable() is only reachable from instrumented code, so we
    //     don't need to worry about HTM transactions.

    // If we are in a prefix or postfix transaction, then the easiest thing to
    // do is just abort it
    if (is_rh_postfix_active || is_rh_prefix_active) {
      _xabort(99); // it's not 66, so it takes the same code path as capacity,
                   // and capacity always causes fallback to STM
    }

    // try_irrevoc will return true only if we got the token and quiesced
    if (!epoch.tryIrrevoc(globals.epoch)) {
      abortTx();
    }

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
              globals.htm_counter.val.load(std::memory_order_relaxed))
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
