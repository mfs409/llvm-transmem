#pragma once

#include <atomic>
#include <setjmp.h>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/orec_t.h"
#include "../common/p_status_t.h"
#include "../common/pad_word.h"
#include "../common/platform.h"

/// P_OrecEager is an STM algorithm with the following characteristics:
/// - Uses orecs for encounter-time write locking, optimistic read locking
/// - Uses a global clock (counter) to avoid validation
/// - Performs speculative writes in-place (uses undo)
///
/// P_OrecEager can be customized in the following ways:
/// - Size of orec table
/// - The undo log implementation
/// - EpochManager (quiescence)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (dynamic capture optimization)
template <class ORECTABLE, class UNDOLOG, class EPOCH, class CM,
          class STACKFRAME, class ALLOCATOR>
class P_OrecEager {
  /// Globals is a wrapper around all of the global variables used by OrecEager
  struct Globals {
    /// The table of orecs for concurrency control
    ORECTABLE orecs;

    /// The contention management metadata
    typename CM::Globals cm;

    /// Quiescence support
    typename EPOCH::Globals epoch;
  };

  /// All metadata shared among threads
  static Globals globals;

  /// The transaction's persistent status
  P_status p_status;

  /// Checkpoint lets us reset registers and the instruction pointer in the
  /// event of an abort
  jmp_buf *checkpoint = nullptr;

  /// For managing thread Ids, Quiescence, and Irrevocability
  EPOCH epoch;

  /// Contention manager
  CM cm;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The value of the global clock when this transaction started/validated
  uint64_t start_time;

  /// The lock token used by this thread
  uint64_t my_lock;

  /// all of the orecs this transaction has read
  MiniVector<orec_t *> readset;

  /// all of the orecs this transaction has locked
  MiniVector<orec_t *> lockset;

  /// original values that this transaction overwrote
  UNDOLOG p_undolog;

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

  /// construct a thread's transaction context
  P_OrecEager()
      : p_status(nullptr, &p_undolog, &allocator), epoch(globals.epoch), cm() {
    my_lock = ORECTABLE::make_lockword(epoch.id);
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

      // Get the start time, and put it into the epoch.  epoch.onBegin will wait
      // until there are no irrevocable transactions.
      start_time = globals.orecs.get_time();
      epoch.onBegin(globals.epoch, start_time);

      // Notify CM of intention to start.  If return true, become irrevocable
      if (cm.beforeBegin(globals.cm)) {
        std::terminate();
      }
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      // fast-path for read-only transactions must still quiesce before freeing
      if (lockset.empty()) {
        // possibly flush writes to captured memory
        if (allocator.p_precommit()) {
          pmem_sfence();
        }
        allocator.commitMallocs();
        epoch.clearEpoch(globals.epoch);
        readset.clear();
        cm.afterCommit(globals.cm);
        epoch.quiesce(globals.epoch, start_time);
        allocator.commitFrees();
        deferredActions.onCommit();
        frame.onCommit();
        return;
      }

      // Commit a writer transaction.  First, clwb the allocator metadata.  We
      // do it now to save a fence
      allocator.p_precommit();

      // get a commit time (includes memory fence)
      uint64_t end_time = globals.orecs.increment_get();

      // validate if there were any intervening commits
      // NB: on relaxed architectures, there will be unnecessary fences
      if (end_time != start_time + 1) {
        for (auto o : readset) {
          uint64_t v = o->curr;
          if (v > start_time && v != my_lock) {
            abortTx();
          }
        }
      }

      allocator.commitMallocs(); // has a fence
      // At this point, the transaction is committed, and all its state has been
      // flushed.  Mark self done:
      p_status.on_eager_commit();

      // depart epoch table (fence) and then release locks
      // NB: these stores may result in unnecessary fences
      epoch.clearEpoch(globals.epoch);
      for (auto o : lockset) {
        o->curr = end_time;
      }

      // clear lists.  Quiesce before freeing
      p_undolog.p_clear();
      lockset.clear();
      readset.clear();
      cm.afterCommit(globals.cm);
      epoch.quiesce(globals.epoch, end_time);
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

  /// Transactional read
  template <typename T> T read(T *addr) {
    // No instrumentation if on stack or we're irrevocable
    if (accessDirectly(addr)) {
      return *addr;
    }

    // get the orec addr, then start loop to read a consistent value
    orec_t *o = globals.orecs.get(addr);
    while (true) {
      // read the orec, then location, then orec
      local_orec_t pre, post;
      pre.all = o->curr; // fenced read of o->curr
      T tmp = *addr;
      if (pre.all == my_lock)
        return tmp;       // if caller has lock, we're done
      post.all = o->curr; // fenced read of o->cirr

      // common case: new read to an unlocked, old location
      if ((pre.all == post.all) && (pre.all <= start_time)) {
        readset.push_back(o);
        return tmp;
      }

      // abort if locked
      if (post.fields.lock) {
        abortTx();
      }

      // validate and then update start time, because orec is unlocked but too
      // new, then try again
      uintptr_t newts = globals.orecs.get_time();
      epoch.setEpoch(globals.epoch, newts);
      validate();
      start_time = newts;
    }
  }

  /// Transactional write
  template <typename T> void write(T *addr, T val) {
    // No instrumentation if on stack or we're irrevocable
    if (accessDirectly(addr)) {
      *addr = val;
      allocator.p_flushOnCapturedStore(addr);
      return;
    }

    // On first write, we need to mark self as having a valid undo log
    if (p_undolog.empty()) {
      p_status.on_eager_start();
    }

    // get the orec addr, then start loop to ensure a consistent value
    orec_t *o = globals.orecs.get(addr);
    while (true) {
      // read the orec BEFORE we do anything else
      local_orec_t pre;
      pre.all = o->curr;
      // If lock unheld and not too new, acquire; abort on fail to acquire
      if (pre.all <= start_time) {
        if (!o->curr.compare_exchange_strong(pre.all, my_lock)) {
          abortTx();
        }
        lockset.push_back(o);
        o->prev = pre.all; // for easy undo on abort... Cf. incarnation numbers
      }

      // If lock held by me, all good
      else if (pre.all == my_lock) {
      }

      // If lock held by other, abort
      else if (pre.fields.lock) {
        abortTx();
      }

      // Lock unheld, but too new... validate and then go to top
      else {
        uintptr_t newts = globals.orecs.get_time();
        epoch.setEpoch(globals.epoch, newts);
        validate();
        start_time = newts;
        continue;
      }

      // We have the lock exclusively, and can write:
      // log it (includes clwb and fence)
      p_undolog.p_push_back(addr);

      // update it and clwb it
      *addr = val;
      pmem_clwb(addr);
      return;
    }
  }

  /// Instrumentation to become irrevocable in-flight.  This is essentially an
  /// early commit
  void becomeIrrevocable() { /*std::terminate();*/ }

  /// Register an action to run after transaction commit
  void registerCommitHandler(void (*func)(void *), void *args) {
    deferredActions.registerHandler(func, args);
  }

private:
  /// Validation.  We need to make sure that all orecs that we've read
  /// have timestamps older than our start time, unless we locked those orecs.
  /// If we locked the orec, we did so when the time was smaller than our start
  /// time, so we're sure to be OK.
  void validate() {
    // NB: on relaxed architectures, we may have unnecessary fences here
    for (auto o : readset) {
      local_orec_t lo;
      lo.all = o->curr;
      if (lo.all > start_time && lo.all != my_lock) {
        abortTx();
      }
    }
  }

  /// Abort the transaction. We must handle mallocs and frees, and we need to
  /// ensure that the OrecEager object is in an appropriate state for starting a
  /// new transaction.  Note that we *will* call beginTx again, unlike libITM.
  void abortTx() {
    if (p_undolog.size()) {
      // undo any writes
      for (auto it = p_undolog.rbegin(), e = p_undolog.rend(); it != e; ++it) {
        it->p_restoreValue(); // includes clwb
      }
      // make sure restoration clwbs precede state change
      pmem_sfence();

      // since we had writes, we must also be ACTIVE.  Mark self inactive
      p_status.on_eager_abort();
    }
    // At this point, we can exit the epoch so that other threads don't have to
    // wait on this thread
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // release the locks and bump version numbers by one... track the highest
    // version number we write, in case it is greater than timestamp.val
    uintptr_t max = 0;
    for (auto o : lockset) {
      uint64_t val = o->prev + 1;
      o->curr = val;
      max = (val > max) ? val : max;
    }
    // if we bumped a version number to higher than the timestamp, we need to
    // increment the timestamp to preserve the invariant that the timestamp
    // val is >= all unocked orecs' values
    uintptr_t ts = globals.orecs.get_time();
    if (max > ts)
      globals.orecs.increment();

    // reset all lists
    readset.clear();
    p_undolog.p_clear();
    lockset.clear();
    allocator.onAbort(); // this reclaims all mallocs
    deferredActions.onAbort();
    frame.onAbort();
    longjmp(*checkpoint, 1); // Takes us back to calling beginTx()
  }

  /// Check if the given address is on the thread's stack, and hence does not
  /// need instrumentation.  Note that if the thread is irrevocable, we also say
  /// that instrumentation is not needed.  Also, the allocator may suggest
  /// skipping instrumentation.
  bool accessDirectly(void *ptr) {
    if (allocator.checkCaptured(ptr))
      return true;
    return frame.onStack(ptr);
  }
};
