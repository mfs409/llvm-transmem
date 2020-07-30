#pragma once

#include <atomic>
#include <setjmp.h>

#include "../common/deferred.h"
#include "../common/pad_word.h"
#include "../common/platform.h"

/// RingMW is the "multi writer in-order" variant of the RingSTM algorithm.  It
/// has the following characteristics:
/// - Uses a bit vector to track the read and write sets
/// - Performs speculative writes out of place (uses redo)
/// - Allows writers to write-back simultaneously, as long as there are not WaW
///   conficts.
///
/// RingMW can be customized in the following ways:
/// - Ring configuration (# vectors, # bits)
/// - Bit filter (e.g., for SSE)
/// - Redo Log (data structure and granularity of chunks)
/// - EpochManager (quiescence and irrevocability)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (to become irrevocable on too many allocations)
template <int RING_ELEMENTS, class BITFILTER, class REDOLOG, class EPOCH,
          class CM, class STACKFRAME, class ALLOCATOR>
class RingMW {
  /// Globals is a wrapper around all of the global variables used by RingMW
  struct Globals {
    /// The global timestamp, for assigning commit orders and reducing
    /// validation
    pad_word_t timestamp;

    /// This indicates the last transaction that finished its commit
    pad_word_t last_complete;

    /// This indicates the last transaction to insert into the ring
    pad_word_t last_init;

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

  /// The read set (bitvector) of this transaction
  BITFILTER *rf;

  // a redolog, since this is a lazy TM
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

  /// construct a thread's transaction context by zeroing its nesting depth and
  /// giving it an ID.
  RingMW() : epoch(globals.epoch), cm() {
    // NB: this allocation strategy is necessary since (optional) SSE introduces
    // alignment requirements
    wf = (BITFILTER *)BITFILTER::filter_alloc(sizeof(BITFILTER));
    wf->clear();
    rf = (BITFILTER *)BITFILTER::filter_alloc(sizeof(BITFILTER));
    rf->clear();
  }

  /// Instrumentation to run at the beginning of a transaction boundary.
  void beginTx(jmp_buf *b) {
    // onBegin == false -> flat nesting
    if (frame.onBegin()) {
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
        rf->clear();
        epoch.clearEpoch(globals.epoch);
        cm.afterCommit(globals.cm);
        epoch.quiesce(globals.epoch, start_time);
        allocator.onCommit();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // get a commit time, but only succeed in the CAS if this transaction
      // is still valid
      uint64_t end_time;
      do {
        end_time = globals.timestamp.val;
        // get the latest ring entry, return if we've seen it already
        if (end_time != start_time) {
          // wait for the latest entry to be initialized
          while (globals.last_init.val < end_time)
            spin64();

          // intersect against all new entries
          for (uint64_t i = end_time; i >= start_time + 1; i--)
            if (globals.ring_wf[i % RING_ELEMENTS].intersect(rf)) {
              abortTx();
            }

          // detect ring rollover: start.ts must not have changed
          if (globals.timestamp.val >= (start_time + RING_ELEMENTS)) {
            abortTx();
          }

          // ensure this tx doesn't look at this entry again
          start_time = end_time;
        }
      } while (!globals.timestamp.val.compare_exchange_strong(end_time,
                                                              end_time + 1));

      // copy the bits over
      globals.ring_wf[(end_time + 1) % RING_ELEMENTS].fastcopy(wf);

      // setting this says "the bits are valid"
      globals.last_init.val = end_time + 1;

      // Handle WaW conflicts by waiting for conflicting predecessors to
      // write back
      for (uint64_t i = end_time; globals.last_complete.val < i; i--) {
        if (globals.ring_wf[i % RING_ELEMENTS].intersect(wf))
          while (globals.last_complete.val < i)
            spin64();
      }

      // Run redo log
      redolog.writeback_atomic();

      // depart epoch table (fence) and then mark writeback complete
      epoch.clearEpoch(globals.epoch);

      // Mark transaction complete (in order)
      while (globals.last_complete.val < end_time) {
        spin64();
      }
      globals.last_complete.val = end_time + 1;

      // clear lists.  Quiesce before freeing
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
    // log the value to the read filter
    rf->add(ptr);

    // get the latest initialized ring entry, return if we've seen it already
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

  /// Transactional write
  template <typename T> void write(T *ptr, T val) {
    if (accessDirectly(ptr)) {
      *ptr = val;
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

    // try_irrevoc will return true only if we got the token and quiesced
    if (!epoch.tryIrrevoc(globals.epoch)) {
      abortTx();
    }

    // Quick validation, since we know the lock is even and immutable
    if (!check_valid(globals.last_init.val)) {
      epoch.onCommitIrrevoc(globals.epoch);
      abortTx();
    }

    // replay redo log
    redolog.writeback_nonatomic();

    // clear lists
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
    // intersect against all new entries
    bool conflict = false;
    for (uint64_t i = my_index; i >= start_time + 1; i--) {
      conflict |= (globals.ring_wf[i % RING_ELEMENTS].intersect(rf));
    }
    if (conflict) {
      return false;
    }

    // detect ring rollover: start.ts must not have changed
    if (globals.timestamp.val > (start_time + RING_ELEMENTS)) {
      return false;
    }

    // ensure this tx doesn't look at this entry again, unless it hasn't written
    // back yet.
    uint64_t last_index = globals.last_complete.val;
    if (my_index < last_index)
      last_index = my_index;

    start_time = last_index;
    epoch.setEpoch(globals.epoch, start_time);
    return true;
  }

  /// Abort the transaction
  void abortTx() {
    // We can exit the Epoch right away, so that other threads don't have to
    // wait on this thread.
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // reset all lists
    redolog.reset();
    rf->clear();
    wf->clear();
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
