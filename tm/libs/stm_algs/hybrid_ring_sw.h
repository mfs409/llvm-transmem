#pragma once

#include <atomic>
#include <setjmp.h>
#include <x86intrin.h>

#include "../common/deferred.h"
#include "../common/pad_word.h"
#include "../common/platform.h"

/// HybridRingSW is hybrid variant of the RingSW algorithm.  It has the 
/// following characteristics:
/// - It uses 2 counter for concurrency control: for STM to notify HTM of
///   commit and for STM to notify HTM of begin
/// - HTM cannot commit during STM writeback
/// - Uses a bit vector to track the read and write sets
/// - Performs speculative writes out of place (uses redo)
/// - Only allows one writer to commit at a time
///
/// ReducedHardwareRingSW can be customized in the following ways:
/// - RETRY (HTM attempts that fail before falling back to STM)
/// - Ring configuration (# vectors, # bits)
/// - Bit filter (e.g., for SSE)
/// - Redo Log (data structure and granularity of chunks)
/// - EpochManager (quiescence and irrevocability)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to become irrevocable on too many allocations)
template <int RETRY, int RING_ELEMENTS, class BITFILTER, class REDOLOG,
          class EPOCH, class CM, class STACKFRAME, class ALLOCATOR>
class HybridRingSW {
  /// Globals is a wrapper around all of the global variables used by
  /// HybridRingSW
  struct Globals {
    /// The global timestamp, for assigning commit orders and reducing
    /// validation
    pad_word_t timestamp;

    /// This indicates the last transaction that finished its commit
    pad_word_t last_complete;

    /// This indicates the last transaction to insert into the ring
    pad_word_t last_init;

    /// The counter used by STM to notify HTM of the begin of a Sw Tx
    pad_word_t stm_begin_counter;

    /// The counter used by STM to notify HTM of the commit of a Sw Tx
    pad_word_t stm_commit_counter;

    /// The counter used to track the number of Sw Tx commited
    pad_word_t stm_transactions_counter;

    /// The counter used to track the number of instrumented Hw Tx commited
    pad_word_t instrumented_htm_transactions_counter;

    /// The counter used to track the number of uninstrumented Hw Tx commited
    pad_word_t uninstrumented_htm_transactions_counter;

    /// The global ring, which contains writeset bloom filters from a
    /// finite number of the most recent committed and pending transactions
    BITFILTER ring_wf[RING_ELEMENTS] __attribute__((aligned(16)));

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

  /// The value of the global clock when this transaction started/validated
  uint64_t start_time = 0;

  /// The write set (bitvector) of this transaction
  BITFILTER *wf;

  /// Indicate if the transaction is running in HTM mixed mode
  bool instrumented_htm_path;

  /// Indicate if the transaction is running in HTM fast mode
  bool uninstrumented_htm_path;

  /// The read set (bitvector) of this transaction
  BITFILTER *rf;

  /// A redolog, since this is a lazy TM
  REDOLOG redolog;

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

  /// Construct a thread's transaction context by zeroing its nesting depth and
  /// giving it an ID
  HybridRingSW() : epoch(globals.epoch), cm(), instrumented_htm_path(false),
                   uninstrumented_htm_path(false) {
    // NB: this allocation strategy is necessary since (optional) SSE introduces
    // alignment requirements
    wf = (BITFILTER *)BITFILTER::filter_alloc(sizeof(BITFILTER));
    wf->clear();
    rf = (BITFILTER *)BITFILTER::filter_alloc(sizeof(BITFILTER));
    rf->clear();
  }

  /// Begin Hw Tx by trying the uninstrumented path first and, if it fails,
  /// then try the instrumented path. If transactions cannot be completed in
  /// hardware, then fall back to software.
  bool beginHTM(void) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // Notify CM, in case we're supposed to wait, but ignore any suggestion to
      // become irrevocable
      cm.beforeBegin(globals.cm);

      // Try the uninstrumented path first
      if (beginUninstrumentedHTM()) {
        return true;
      }

      // Record how many times HTM has aborted
      int retry_counter = 0;
      uint64_t status, counter;
    retry:
      // Try the instrumented path. Wait if STM is doing writeback or
      // irrevocable STM is running. This sort of spinning should not cost our
      // HTM attempts.
      do {
        counter =
          globals.stm_commit_counter.val.load(std::memory_order_relaxed);
      } while (counter > 0 || epoch.existIrrevoc(globals.epoch));

      // Start hardware transaction
      status = _xbegin();
      if (status == _XBEGIN_STARTED) {
        // Abort if STM is doing writeback or irrevocable STM is running
        if (globals.stm_commit_counter.val > 0 ||
            epoch.existIrrevoc(globals.epoch)) {
          _xabort(66);
        }
        instrumented_htm_path = true;
        // NB: epoch, and allocator *do not know about this transaction*
        return false;
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
        instrumented_htm_path = false;
        // Depart CM, so that we can re-enter it from STM path
        cm.afterAbort(globals.cm, epoch.id);
        return false;
      }
    }
    // If we're not nested, take the appropriate path for the state we're in...
    //
    // [mfs] If we have nested transactions, this is not exactly going to work,
    //       but we aren't going to worry about it for now.
    return instrumented_htm_path;
  }

private:
  /// HTM path without instrumentation
  bool beginUninstrumentedHTM() {
    // Record how many times HTM has aborted
    int retry_counter = 0;
    uint64_t status;
    retry:
      // Start hardware transaction
      status = _xbegin();
      if (status == _XBEGIN_STARTED) {
        // Abort if STM is doing writeback or irrevocable STM is running
        if (globals.stm_begin_counter.val > 0) {
          _xabort(66);
        }
        uninstrumented_htm_path = true;
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
        uninstrumented_htm_path = false;
        return false;
      }
  }

public:
  /// Instrumentation to run at the beginning of an STM transaction.
  void beginSTM(jmp_buf *b) {
    if (instrumented_htm_path || uninstrumented_htm_path)
      return;

    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
      // increase the stm_begin_counter, which disable the start of
      // uninstrumented HTM transactions
      ++globals.stm_begin_counter.val;

      // Save the checkpoint and set the stack bottom
      checkpoint = b;
      frame.setBottom(b);

      // Start logging allocations
      allocator.onBegin();

      // Start time is when the last txn completed
      start_time = globals.last_complete.val;

      epoch.onBegin(globals.epoch, start_time);

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
      // uninstrumented HTM path can just commit
      if (uninstrumented_htm_path) {
        // Abort if STM is running
        if (globals.stm_begin_counter.val > 0) {
          _xabort(66);
        }
        _xend();

        ++globals.uninstrumented_htm_transactions_counter.val;

        cm.afterCommit(globals.cm);
        deferredActions.onCommit();
        frame.onCommit();
        // NB: Recall that HTM is not visible to epoch
        return;
      }
      // To commit from instrumented HTM path, update the ring, then commit
      if (instrumented_htm_path) {
        // NB: We subscribed to stm_commit_counter at begin time, and because
        //     of that, we know that if some STM transaction "completely"
        //     finished (e.g., did writeback) during this HTM transaction, then
        //     this HTM transaction would have aborted.  But we still have one
        //     problem, which is that an STM transaction could be on the brink
        //     of committing (e.g., linearized, but not yet even done
        //     initializing its ring entry).  In that case, this HTM
        //     transaction, by incrementing globals.timestamp, would happen
        //     *after* it, but wouldn't see its writes, and hence could be
        //     invalid. this whole problem can be solved like this.
        if (globals.timestamp.val != globals.last_complete.val) {
          _xabort(66);
        }

        // Update timestamp so we can reserve the spot.
        uint64_t end_time = globals.timestamp.val;
        ++globals.timestamp.val;

        // Copy the bits over
        globals.ring_wf[(end_time + 1) % RING_ELEMENTS].fastcopy(wf);

        // Setting this says "the bits are valid"
        globals.last_init.val = end_time + 1;

        // Setting this says "writeback is done"... of course, in HTM, writeback
        // is free :)
        globals.last_complete.val = end_time + 1;

        _xend();

        ++globals.instrumented_htm_transactions_counter.val;

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
        wf->clear();
        cm.afterCommit(globals.cm);
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        // NB: Recall that HTM is not visible to epoch
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
      // Fast-path for read-only transactions must still quiesce before freeing
      if (redolog.isEmpty()) {
        // Decrease the stm_begin_counter, which enables the start of
        // uninstrumented HTM transactions
        --globals.stm_begin_counter.val;
        rf->clear();
        epoch.clearEpoch(globals.epoch);
        cm.afterCommit(globals.cm);
        epoch.quiesce(globals.epoch, start_time);
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // Get a commit time, but only succeed in the CAS if this transaction
      // is still valid
      uint64_t end_time;
      do {
        end_time = globals.timestamp.val;
        // Get the latest ring entry, return if we've seen it already
        if (end_time != start_time) {
          // Wait for the latest entry to be initialized
          while (globals.last_init.val < end_time)
            spin64();

          // Intersect against all new entries
          for (uint64_t i = end_time; i >= start_time + 1; i--)
            if (globals.ring_wf[i % RING_ELEMENTS].intersect(rf)) {
              abortTx();
            }

          // Wait for newest entry to be wb-complete before continuing
          while (globals.last_complete.val < end_time) {
            spin64();
          }

          // Detect ring rollover: start.ts must not have changed
          if (globals.timestamp.val >= (start_time + RING_ELEMENTS)) {
            abortTx();
          }

          // Ensure this tx doesn't look at this entry again
          start_time = end_time;
        }
      } while (!globals.timestamp.val.compare_exchange_strong(end_time,
                                                              end_time + 1));

      // Copy the bits over
      globals.ring_wf[(end_time + 1) % RING_ELEMENTS].fastcopy(wf);

      // Setting this says "the bits are valid"
      globals.last_init.val = end_time + 1;

      // Increase the stm_commit_counter, which makes concurrent HTMs abort
      ++globals.stm_commit_counter.val;

      // We're committed... run redo log, then mark ring entry COMPLETE
      redolog.writeback_atomic();

      // Depart epoch table (fence) and then mark writeback complete
      epoch.clearEpoch(globals.epoch);

      // Decrease the stm_commit_counter, which enables concurrent HTMs to start
      --globals.stm_commit_counter.val;

      ++globals.stm_transactions_counter.val;

      // Decrease the stm_begin_counter, which enables the start of
      // uninstrumented HTM transactions
      --globals.stm_begin_counter.val;

      globals.last_complete.val = end_time + 1;

      // Clear lists.  Quiesce before freeing
      redolog.reset();
      rf->clear();
      wf->clear();
      cm.afterCommit(globals.cm);
      epoch.quiesce(globals.epoch, end_time);
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
    if (accessDirectly(ptr) || instrumented_htm_path || uninstrumented_htm_path) {
      return *ptr;
    } else {
      // Lookup in redo log to populate ret.  Note that prior casting can lead
      // to ret having only some bytes properly set
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

      // Log the value to the read filter
      rf->add(ptr);

      // Get the latest initialized ring entry, return if we've seen it already
      uint64_t my_index = globals.last_init.val;
      if (__builtin_expect(my_index != start_time, false)) {
        if (!check_valid(my_index)) {
          abortTx();
        }
      }

      // If redolog was a partial hit, reconstruction is needed
      if (!found_mask) {
        return from_mem;
      }
      REDOLOG::reconstruct(from_mem, ret, found_mask);
      return ret;
    }
  }

  /// Transactional write
  template <typename T> void write(T *ptr, T val) {
    if (accessDirectly(ptr) || uninstrumented_htm_path) {
      *ptr = val;
    } else if (instrumented_htm_path) {
      *ptr = val;
      wf->add(ptr);
    } else {
      redolog.insert(ptr, val);
      wf->add(ptr);
    }
  }

  /// Instrumentation to become irrevocable... This is essentially an early
  /// commit
  void becomeIrrevocable() {
    // Immediately return if we are already irrevocable
    if (epoch.isIrrevoc()) {
      return;
    }

    // Try_irrevoc will return true only if we got the token and quiesced
    if (!epoch.tryIrrevoc(globals.epoch)) {
      abortTx();
    }

    // Quick validation, since we know the lock is even and immutable
    if (!check_valid(globals.last_init.val)) {
      epoch.onCommitIrrevoc(globals.epoch);
      abortTx();
    }

    // Replay redo log
    redolog.writeback_nonatomic();

    // Decrease the stm_begin_counter, which enables the start of
    // uninstrumented HTM transactions
    --globals.stm_begin_counter.val;

    // Clear lists
    allocator.onCommit();
    rf->clear();
    wf->clear();
    redolog.reset();
  }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }

private:
  /// Check the validity of a transaction by doing bitfilter intersections
  bool check_valid(uint64_t my_index) {
    // Intersect against all new entries
    bool conflict = false;

    for (uint64_t i = my_index; i >= start_time + 1; i--) {
      conflict |= (globals.ring_wf[i % RING_ELEMENTS].intersect(rf));
    }
    if (conflict) {
      return false;
    }

    // Wait for newest entry to be writeback-complete before returning
    while (globals.last_complete.val < my_index)
      spin64();

    // Detect ring rollover: start.ts must not have changed
    if (globals.timestamp.val > (start_time + RING_ELEMENTS)) {
      return false;
    }

    // Ensure this tx doesn't look at this entry again
    start_time = my_index;
    epoch.setEpoch(globals.epoch, start_time);
    return true;
  }

  /// Abort the transaction
  void abortTx() {
    // We can exit the Epoch right away, so that other threads don't have to
    // wait on this thread.
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // Decrease the stm_begin_counter, which enables the start of
    // uninstrumented HTM transactions
    --globals.stm_begin_counter.val;

    // Reset all lists
    redolog.reset();
    rf->clear();
    wf->clear();
    allocator.onAbort(); // This reclaims all mallocs
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