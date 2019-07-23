#pragma once

#include <atomic>
#include <setjmp.h>

#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/orec_t.h"
#include "../common/p_status_t.h"
#include "../common/pad_word.h"
#include "../common/platform.h"

/// P_OrecMixed is an STM algorithm with the following characteristics:
/// - Uses orecs for encounter-time locking, optimistic read locking
/// - Uses a global clock (counter) to avoid validation
/// - Performs speculative writes out of place (uses redo)
///
/// P_OrecMixed can be customized in the following ways:
/// - Size of orec table
/// - Redo Log (data structure and granularity of chunks)
/// - EpochManager (quiescence)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (captured memory)
template <class ORECTABLE, class REDOLOG, class EPOCH, class CM,
          class STACKFRAME, class ALLOCATOR>
class P_OrecMixed {
  /// Globals is a wrapper around all of the global variables used by OrecMixed
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

  /// For managing thread IDs, Quiescence, and Irrevocability
  EPOCH epoch;

  /// Contention manager
  CM cm;

  /// For managing the stack frame
  STACKFRAME frame;

  /// The value of the global clock when this transaction started/validated
  uint64_t start_time = 0;

  /// The lock token used by this thread
  uint64_t my_lock;

  /// all of the orecs this transaction has read
  MiniVector<orec_t *> readset;

  /// all of the orecs this transaction has locked
  MiniVector<orec_t *> lockset;

  /// a redolog, since this is a lazy TM
  REDOLOG p_redolog;

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
  /// giving it an ID.  We also cache its lock token.
  P_OrecMixed()
      : p_status(nullptr, &p_redolog, &allocator), epoch(globals.epoch), cm() {
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
        becomeIrrevocable();
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
      // Commit a writer transaction: clwb any stores to captured memory, and
      // persist the redo log
      allocator.p_precommit();
      p_redolog.p_precommit();

      // get a commit time (includes memory fence)
      uint64_t end_time = globals.orecs.increment_get();

      // validate if there were any intervening commits
      if (end_time != start_time + 1) {
        for (auto i : readset) {
          uint64_t v = i->curr;
          if (v > start_time && v != my_lock) {
            abortTx();
          }
        }
      }

      // NB: CAS was an SFENCE, so no pfence needed here

      // Mark self committed, according to redolog's writeback mode
      if (!p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        // Since writeback does clwb while holding lock, we set self to REDO
        p_status.before_redo_writeback();
      } else {
        // Since writeback doesn't clwb, we set self to a timestamp
        p_status.before_redo_writeback_opt(end_time);
      }

      // writeback, mark done, release lock, post-writeback
      //
      // NB: if we have delayed clwbs of program memory, then we can't mark self
      //     done until *after* the clwbs.  p_postcommit() will be a nop in that
      //     case.
      p_redolog.p_writeback();
      allocator.commitMallocs(); // has a fence
      if (!p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        p_status.after_redo_writeback();
      }

      // depart epoch table (fence) and then release locks
      // NB: these stores may result in unnecessary fences
      epoch.clearEpoch(globals.epoch);
      releaseLocks(end_time);

      p_redolog.p_postcommit();
      if (p_redolog.WRITEBACK_NEEDS_TIMESTAMP) {
        p_status.after_redo_writeback();
      }

      // clear lists.  Quiesce before freeing
      p_redolog.clear();
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

    orec_t *o = globals.orecs.get(addr);
    local_orec_t tmp;
    tmp.all = o->curr;
    typename REDOLOG::mask_t found_mask = 0;
    T ret;
    if (tmp.all == my_lock) {
      // Lookup in redo log to populate ret.  Note that prior casting can lead
      // to ret having only some bytes properly set
      found_mask = p_redolog.get_partial(addr, ret);
      // If we found all the bytes in the redo log, then it's easy
      typename REDOLOG::mask_t desired_mask = REDOLOG::create_mask(sizeof(T));
      if (desired_mask == found_mask) {
        return ret;
      }

      // Fast path, no need to validate!
      if (found_mask == 0)
        return *addr;
    }

    // get the orec addr, then start loop to read a consistent value
    T from_mem;
    while (true) {
      // read the orec, then location, then orec
      local_orec_t pre, post;
      pre.all = o->curr; // fenced read of o->curr
      from_mem = *addr;
      post.all = o->curr; // fenced read of o->curr

      // common case: new read to an unlocked, old location
      if ((pre.all == post.all) && (pre.all <= start_time)) {
        readset.push_back(o);
        break;
      }

      // other threads locked this orec
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

    // If redolog was a partial hit, reconstruction is needed
    if (!found_mask) {
      return from_mem;
    }
    REDOLOG::reconstruct(from_mem, ret, found_mask);
    return ret;
  }

  /// Transactional write
  template <typename T> void write(T *addr, T val) {
    // No instrumentation if on stack or we're irrevocable
    if (accessDirectly(addr)) {
      *addr = val;
      allocator.p_flushOnCapturedStore(addr);
      return;
    }
    // get the orec addr
    orec_t *o = globals.orecs.get(addr);
    while (true) {
      // read the orec BEFORE we do anything else
      local_orec_t pre;
      pre.all = o->curr;
      // If lock unheld, acquire; abort on fail to acquire
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
      break;
    }
    if (p_redolog.insert_if(addr, val)) {
      return;
    }
    // We are doing a Coarse logging non-WaW read.  Still, we have the lock, so
    // consistent reading is free.
    uint64_t fill[REDOLOG::REDO_WORDS];
    uintptr_t *start_addr =
        (uintptr_t *)(((uintptr_t)addr) & ~(REDOLOG::REDO_CHUNKSIZE - 1));
    memcpy(&fill, start_addr, REDOLOG::REDO_CHUNKSIZE);
    p_redolog.insert_with(addr, val, fill);
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

    // NB: The common case is "no abort", so we don't put the branches inside
    // the loop.  If we end up aborting, the extra orec checks are kind of like
    // backoff.
    bool to_abort = false;
    for (auto o : readset) {
      local_orec_t lo;
      lo.all = o->curr;
      to_abort |= (lo.all > start_time && lo.all != my_lock);
    }
    if (to_abort) {
      abortTx();
    }
  }

  /// Abort the transaction
  void abortTx() {
    // We can exit the Epoch right away, so that other threads don't have to
    // wait on this thread.
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // release any locks held by this thread
    // NB: possible extra fences
    for (auto o : lockset) {
      if (o->curr == my_lock) {
        o->curr.store(o->prev);
      }
    }

    // reset all lists
    readset.clear();
    p_redolog.clear();
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

  /// Release the locks held by this transaction
  void releaseLocks(uint64_t end_time) {
    // NB: possible extra fences
    for (auto o : lockset) {
      if (o->curr == my_lock) {
        o->curr = end_time;
      }
    }
  }
};
