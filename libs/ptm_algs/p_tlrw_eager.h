#pragma once

#include <atomic>
#include <exception>
#include <setjmp.h>

#include "../common/bytelock_t.h"
#include "../common/deferred.h"
#include "../common/minivector.h"
#include "../common/p_status_t.h"
#include "../common/pad_word.h"
#include "../common/platform.h"
#include "../common/undo_t.h"

/// P_TLRWEager is an STM algorithm with the following characteristics:
/// - Uses "bytelocks" as readers/writer locks for encounter-time write locking
///   and pessimistic read locking
/// - Performs speculative writes in-place (uses undo)
/// - Uses a wait-then-self-abort strategy to avoid deadlocks
/// - Inherently privatization safe and validation-free
///
/// TLRWEager can be customized in the following ways:
/// - Size of bytelock table (and bytelock size / max thread count)
/// - The undo log implementation
/// - EpochManager (but quiescence isn't needed)
/// - Contention manager
/// - Stack Frame (to bring some of the caller frame into tx scope)
/// - Allocator (dynamic capture optimization)
/// - Deadlock avoidance tuning (read tries, read spins, write tries, write
///   spins)
/// - Pipelining of stores, so that a store does not reach memory until the next
///   API call.  This saves a fence for PTM.
///
/// Please see the STM TLRWEager implementation for more details, particularly
/// with regard to tuning contention management
template <class BYTELOCKTABLE, class UNDOLOG, class EPOCH, class CM,
          class STACKFRAME, class ALLOCATOR, int READ_TRIES, int READ_SPINS,
          int WRITE_TRIES, int WRITE_SPINS, bool PIPELINE_STORES>
class P_TLRWEager {
  /// Globals is a wrapper around all of the global variables used by TLRWEager
  struct Globals {
    /// The table of bytelocks for concurrency control
    BYTELOCKTABLE bytelocks;

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

  /// The slot used by this thread
  uint64_t my_slot;

  /// all of the bytelocks this transaction has read-locked
  MiniVector<typename BYTELOCKTABLE::bytelock_t *> readset;

  /// all of the bytelocks this transaction has write-locked
  MiniVector<typename BYTELOCKTABLE::bytelock_t *> lockset;

  /// original values that this transaction overwrote
  UNDOLOG p_undolog;

  /// The allocator manages malloc, free, and aligned alloc
  ALLOCATOR allocator;

  /// When PIPELINE_STORES is true, we will use this to save the next store
  /// value
  undo_t pipeline_store;

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
  P_TLRWEager()
      : p_status(nullptr, &p_undolog, &allocator), epoch(globals.epoch), cm() {
    my_slot = epoch.id;
    // crash immediately if we have an invalid bytelock
    globals.bytelocks.validate_id(my_slot);
    // initialize the pipelining optimization
    pipeline_store.addr.p = nullptr;
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

      // Wait until there are no irrevocable transactions.
      epoch.onBegin(globals.epoch, 1);

      // Notify CM of intention to start.  If return true, become irrevocable
      if (cm.beforeBegin(globals.cm)) {
        std::terminate();
      }

      // Since TLRW doesn't have any read-only optimizations, we mark self as
      // having a valid undo log now, rather than later
      p_status.on_eager_start();
    }
  }

  /// Instrumentation to run at the end of a transaction boundary.
  void commitTx() {
    // onEnd == false -> flat nesting
    if (frame.onEnd()) {
      // clwb the allocator metadata
      bool need_fence = allocator.p_precommit();

      // writeback and flush any pipelined stores
      if (PIPELINE_STORES) {
        if (pipeline_store.addr.p != nullptr) {
          pipeline_store.restoreValue();
          pmem_clwb(pipeline_store.addr.p);
          pipeline_store.addr.p = nullptr;
          need_fence = true;
        }
      }

      // if !PIPELINE_STORES, then we may need a fence to handle the last store
      // if PIPELINE_STORES and we just took the branch, need a fence
      // if allocator.precommit() did anything, need a fence
      if (!PIPELINE_STORES || need_fence) {
        pmem_sfence();
      }

      // NB: can't use commitMallocs' fence for the above fence, because we need
      // allocator.p_precommit() to precede commitMallocs()
      allocator.commitMallocs(); // has a fence

      // At this point, the transaction is committed, and all its state has been
      // flushed.  Mark self done:
      p_status.on_eager_commit();

      // depart epoch table (fence) and then release locks
      //
      // NB: there may be unnecessary fences in this code
      epoch.clearEpoch(globals.epoch);
      // Drop all write locks
      for (auto bl : lockset) {
        bl->owner.store(0, std::memory_order_relaxed);
      }
      // Drop all read locks
      for (auto bl : readset) {
        bl->readers[my_slot].store(0, std::memory_order_relaxed);
      }
      // clear lists
      p_undolog.p_clear();
      lockset.clear();
      readset.clear();
      cm.afterCommit(globals.cm);
      epoch.quiesce(globals.epoch, 2);
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
    // No instrumentation if on stack
    if (accessDirectly(addr)) {
      return *addr;
    }

    // Get the bytelock addr, and check the easy cases
    auto bl = globals.bytelocks.get(addr);
    if (bl->readers[my_slot] != 0 || bl->owner == (my_slot + 1)) {
      // If we have a pipelined store, it could be to addr, so flush it now
      if (PIPELINE_STORES) {
        if (pipeline_store.addr.p != nullptr) {
          pmem_sfence();
          pipeline_store.restoreValue();
          pmem_clwb(pipeline_store.addr.p);
          pipeline_store.addr.p = nullptr;
        }
      }
      return *addr;
    }

    // We might as well log the lock now.  If we abort, releasing won't be an
    // issue.
    readset.push_back(bl);

    // Do read acquisition in a loop, because we might need to try multiple
    // times
    int tries = 0;
    while (true) {
      // In the best case, we write to the readers slot, and then see that there
      // is no owner.
      bl->readers[my_slot] = 1; // NB: this involves a fence!

      // If we have a pipelined store, flush it now since we just had a fence
      if (PIPELINE_STORES) {
        if (pipeline_store.addr.p != nullptr) {
          pipeline_store.restoreValue();
          pmem_clwb(pipeline_store.addr.p);
          pipeline_store.addr.p = nullptr;
        }
      }

      if (bl->owner == 0) { // NB: this may produce an unnecessary fence
        return *addr;
      }

      // In the best case, the owner is about to finish, and isn't waiting
      // on this thread, so if we release the lock and wait a bit, things
      // clear up
      bl->readers[my_slot] = 0;
      if (++tries == READ_TRIES) {
        abortTx();
      }
      spinX(READ_SPINS);
    }
  }

  /// Transactional write
  template <typename T> void write(T *addr, T val) {
    // No instrumentation if on stack
    if (accessDirectly(addr)) {
      *addr = val;
      allocator.p_flushOnCapturedStore(addr);
      return;
    }

    // get the bytelock addr, and check the easy case
    auto bl = globals.bytelocks.get(addr);
    // Easy: we own it already.  Will still need to undo log it before update...
    if (bl->owner == (my_slot + 1)) {
      // If we have a pipelined store, we need to fence and flush it before the
      // next store
      if (PIPELINE_STORES) {
        if (pipeline_store.addr.p != nullptr) {
          pmem_sfence();
          pipeline_store.restoreValue();
          pmem_clwb(pipeline_store.addr.p);
          pipeline_store.addr.p = nullptr;
        }
      }
    }
    // If we don't own it, we need to acquire without deadlocking...
    else {
      // For deadlock avoidance, we abort immediately if we can't get the
      // owner field
      uintptr_t unheld = 0;
      if (!bl->owner.compare_exchange_strong(unheld, (my_slot + 1))) {
        abortTx();
      }
      lockset.push_back(bl);

      // If we have a pipelined store, we need to  flush it before the next
      // store
      if (PIPELINE_STORES) {
        if (pipeline_store.addr.p != nullptr) {
          pipeline_store.restoreValue();
          pmem_clwb(pipeline_store.addr.p);
          pipeline_store.addr.p = nullptr;
        }
      }

      // Drop our read lock, to make the checks easier
      bl->readers[my_slot] = 0;
      // Having the lock isn't enough... We need to wait for readers to drain
      // out
      int tries = 0;
      while (true) {
        uintptr_t count = globals.epoch.getThreads();
        unsigned char conflicts = 0;
        for (int i = 0; i < count; ++i) {
          conflicts |= bl->readers[i];
        }
        if (!conflicts) {
          break;
        }
        if (++tries == WRITE_TRIES) {
          abortTx();
        }
        spinX(WRITE_SPINS);
      }
    }
    // We have the lock exclusively, and can write:
    // log it (includes clwb and fence)
    p_undolog.p_push_back(addr);

    if (PIPELINE_STORES) {
      // defer the update
      pipeline_store.initFromVal(addr, val);
    } else {
      // update it and clwb it
      *addr = val;
      pmem_clwb(addr);
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
  /// Abort the transaction. We must handle mallocs and frees, and we need to
  /// ensure that the TLRWEager object is in an appropriate state for starting a
  /// new transaction.  Note that we *will* call beginTx again, unlike libITM.
  void abortTx() {
    // undo any writes
    for (auto it = p_undolog.rbegin(), e = p_undolog.rend(); it != e; ++it) {
      it->p_restoreValue(); // includes clwb
    }

    // make sure restoration clwbs precede state change
    pmem_sfence();

    // Mark self inactive
    p_status.on_eager_abort();

    // At this point, we can exit the epoch so that other threads don't have to
    // wait on this thread
    epoch.clearEpoch(globals.epoch);
    cm.afterAbort(globals.cm, epoch.id);

    // Drop all read locks
    for (auto bl : readset) {
      bl->readers[my_slot].store(0, std::memory_order_relaxed);
    }
    // Drop all write locks
    for (auto bl : lockset) {
      bl->owner.store(0, std::memory_order_relaxed);
    }

    // erase any pipeline store state
    if (PIPELINE_STORES) {
      pipeline_store.addr.p = nullptr;
    }

    // reset all lists
    readset.clear();
    p_undolog.p_clear();
    lockset.clear();
    allocator.onAbort();
    deferredActions.onAbort();
    frame.onAbort();
    longjmp(*checkpoint, 1);
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
