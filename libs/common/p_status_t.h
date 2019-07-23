#pragma once

#include <cstdint>

#include "pmem_platform.h"

/// P_status represents the status of a descriptor object, so that recovery code
/// can figure out how to restore the heap to a safe state after a crash during
/// transaction execution.
///
/// NB: since we do not simulate crashes, we are not currently writing recovery
///     code.  However, we must ensure that any transaction code would pay the
///     appropriate overheads so that recovery *could* work.  P_status provides
///     the main required functionality, with the remaining functionality in the
///     undo and redo logging code.
class P_status {
  /// a constant to indicate that the undo log needs to be replayed
  static const uint64_t NEEDS_UNDO = -3;

  /// a constant to indicate that the redo log needs to be replayed (and no
  /// order among redos is needed)
  static const uint64_t NEEDS_REDO = -2;

  /// a constant to indicate that this transaction does not need any redo/undo
  static const uint64_t INACTIVE = -1;

  /// The thread status.  It can be one of the above enum values, or it can be a
  /// commit time, in the event that redo() needs to be ordered.
  ///
  /// NB: When operating on a system with NVM, this field must be allocated on
  ///     the persistent heap.
  uint64_t p_thread_status;

  /// A reference to the redo log, so that the recovery algorithm only needs to
  /// track objects of type P_status.
  ///
  /// NB: When operating on a system with NVM, this field must be allocated on
  ///     the persistent heap.
  void *p_redo_log;

  /// A reference to the redo log, so that the recovery algorithm only needs to
  /// track objects of type P_status.
  ///
  /// NB: When operating on a system with NVM, this field must be allocated on
  ///     the persistent heap.
  void *p_undo_log;

  /// A reference to the allocator, for the recovery algorithm.
  ///
  /// NB: When operating on a system with NVM, this field must be allocated on
  ///     the persistent heap.
  void *p_allocator;

public:
  /// Initialize the thread's persistence status by saving references to its
  /// undo and redo logs, and setting its state to inactive.
  P_status(void *_redo, void *_undo, void *_allocator)
      : p_thread_status(INACTIVE), p_redo_log(_redo), p_undo_log(_undo),
        p_allocator(_allocator) {
    pmem_clwb(&p_redo_log);
    pmem_clwb(&p_undo_log);
    pmem_clwb(&p_allocator);
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// When an eager (undo) PTM transaction starts, we need to set the state to
  /// indicate that the undo log is active
  void on_eager_start() {
    p_thread_status = NEEDS_UNDO;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// When an eager PTM transaction finishes, we mark it inactive so that it
  /// will no longer be undone.  This must be called before releasing locks.
  void on_eager_commit() {
    p_thread_status = INACTIVE;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// When an eager PTM transaction aborts and finishes undoing, we mark it
  /// inactive so that it will no longer be undone.  This must be called before
  /// releasing locks.
  void on_eager_abort() {
    p_thread_status = INACTIVE;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// When a lazy PTM transaction is about to writeback, it has already
  /// committed, so we need to let the recovery code know that the redo long
  /// will need to be replayed.  This must be called after acquiring locks.
  void before_redo_writeback() {
    p_thread_status = NEEDS_REDO;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// When a lazy PTM transaction finishes writeback, we mark it inactive so
  /// that it will no longer be redone.  This must be called before releasing
  /// locks.
  void after_redo_writeback() {
    p_thread_status = INACTIVE;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }

  /// If we want to remove the clwb operations from the critical path, then we
  /// don't actually clwb until after commit.  In that case, we need to put the
  /// commit order of the transaction into its status, so that redos can be
  /// replayed in order.
  void before_redo_writeback_opt(uint64_t commit_order) {
    p_thread_status = commit_order;
    pmem_clwb(&p_thread_status);
    pmem_sfence();
  }
};