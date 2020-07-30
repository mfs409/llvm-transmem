#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>

/// P_UndoLog combines undo_t and minivector to create a persistence-friendly
/// undo log.
///
/// The well-known issue with undo logging is that the undo log must be updated
/// *before* memory is updated, thus requiring a clwb and a fence.  However, if
/// we want a truly recoverable undo log, then we have two additional
/// requirements.
///
/// - Since the undo log is a resizable array, we need to be sure that any
///   resize operation persists correctly, so that the latest log of data
///   remains available to the recovery algorithm.
///
/// - Inserting into a naive undo log would actually entail *two* writes: one to
///   put an entry into the log, and one to update the size of the log.  That's
///   a costly overhead, because it means a *second* fence on each insertion.
///   Instead, we will pad our undo log entries to 64 bytes (a cacheline), align
///   the undo log at 64 bytes, and attach a version number to each undo log
///   entry.  The log itself also has a version number, and version number
///   mismatch indicates an un-initialized entry.  In this way, the recovery
///   algorithm can tell when an entry is valid, without extra fences during
///   execution.
///
/// NB: as a reminder that P_UndoLog operations have persistence overheads, the
///     methods are all prefixed with "P_".
///
/// NB: PMEM is for switching between ADR and eADR
template <class PMEM> class P_UndoLog {
public:
  /// p_logentry_t represents a single entry in an undo log for a persistent TM.
  /// It consists of a pointer, a value, something to indicate the type that is
  /// logged, and a timestamp.
  ///
  /// NB: sizeof(p_logentry_t) is 64, which is convenient for persistence
  struct p_logentry_t {
    /// A union storing the value that has been logged.  Again, we use a union
    /// to avoid casting
    union {
      uint8_t u1;
      uint16_t u2;
      uint32_t u4;
      uint64_t u8;
      float f4;
      double f8;
      long double f16;
      void *p;
    } p_val; // 16 bytes

    /// A union storing the address, so that we can load/store through the
    /// address without casting
    union {
      uint8_t *u1;
      uint16_t *u2;
      uint32_t *u4;
      uint64_t *u8;
      float *f4;
      double *f8;
      long double *f16;
      void **p;
    } p_addr; // 8 bytes

    /// A timestamp for this entry, to help with identifying valid entries
    uint64_t p_timestamp; // 8 bytes

    /// A lightweight enum to represent the type of element logged in this
    /// undo_t. Valid values are 0/1/2/3/4/5/6/7,  for
    /// u8/u16/u32/u64/f32/f64/f80/pointer
    int p_type; // 4 bytes

    /// Padding, to ensure a cache line size.  Coupled with aligned allocation
    /// (see below), this ensures that only one clwb instruction is needed per
    /// insertion when cache lines are 64 bytes.
    char pad[(((sizeof(long double) + sizeof(void *) + sizeof(uint64_t) +
                sizeof(int)) <= CACHELINE_BYTES)
                  ? CACHELINE_BYTES
                  : 2 * CACHELINE_BYTES) -
             sizeof(long double) - sizeof(void *) - sizeof(uint64_t) -
             sizeof(int)];

    /// The MAKE_PLOGENTRYT_FUNCS macro creates the type-specific initialization
    /// functions for a p_logentry_t.  We don't exactly "construct" a
    /// p_logentry_t.  Instead, we build it in place, making sure to get the
    /// unions to behave correctly, and then flush/fence our work when we are
    /// done.
#define MAKE_PLOGENTRYT_FUNCS(TYPE, FIELD, TYPEID)                             \
  void p_initFromAddr(TYPE *a, uint64_t ts) {                                  \
    p_val.FIELD = *a;                                                          \
    p_addr.FIELD = a;                                                          \
    p_type = TYPEID;                                                           \
    p_timestamp = ts;                                                          \
    PMEM::clwb(&p_val); /* change if cache line > 64 */                        \
    PMEM::sfence();                                                            \
  }                                                                            \
  void p_initFromVal(TYPE *a, TYPE v) {                                        \
    p_val.FIELD = v;                                                           \
    p_addr.FIELD = a;                                                          \
    p_type = TYPEID;                                                           \
  }
    MAKE_PLOGENTRYT_FUNCS(uint8_t, u1, 0);
    MAKE_PLOGENTRYT_FUNCS(uint16_t, u2, 1);
    MAKE_PLOGENTRYT_FUNCS(uint32_t, u4, 2);
    MAKE_PLOGENTRYT_FUNCS(uint64_t, u8, 3);
    MAKE_PLOGENTRYT_FUNCS(float, f4, 4);
    MAKE_PLOGENTRYT_FUNCS(double, f8, 5);
    MAKE_PLOGENTRYT_FUNCS(long double, f16, 6);
    MAKE_PLOGENTRYT_FUNCS(void *, p, 7);

    /// Restore an address to the value previously saved.
    void p_restoreValue() {
      switch (p_type) {
      case 0:
        *p_addr.u1 = p_val.u1;
        break;
      case 1:
        *p_addr.u2 = p_val.u2;
        break;
      case 2:
        *p_addr.u4 = p_val.u4;
        break;
      case 3:
        *p_addr.u8 = p_val.u8;
        break;
      case 4:
        *p_addr.f4 = p_val.f4;
        break;
      case 5:
        *p_addr.f8 = p_val.f8;
        break;
      case 6:
        *p_addr.f16 = p_val.f16;
        break;
      case 7:
        *p_addr.p = p_val.p;
        break;
      }
      // flush the store, but don't fence yet
      PMEM::clwb(p_addr.p);
    }
  };

private:
  /// The maximum number of things this UndoLog can store without triggering
  /// a resize
  unsigned long capacity;

  /// The number of items currently stored in the UndoLog
  unsigned long count;

  /// Storage for the items in the array
  p_logentry_t *p_items;

  /// Timestamp, for tracking invalid undo log entries
  uint64_t p_timestamp;

  /// Resize the array of items, and move current data into it
  void p_expand() {
    // create space for new array and zero it
    capacity *= 2;
    char *temp =
        (char *)aligned_alloc(CACHELINE_BYTES, sizeof(p_logentry_t) * capacity);
    memset(temp, 0, sizeof(p_logentry_t) * capacity);
    // Copy everything over, persist it
    memcpy(temp, p_items, sizeof(p_logentry_t) * count);
    for (unsigned long i = 0; i < capacity; i += CACHELINE_BYTES) {
      PMEM::clwb(temp + i);
    }
    PMEM::sfence();
    // install pointer to new data, delete old data
    p_logentry_t *old = p_items;
    p_items = (p_logentry_t *)temp;
    PMEM::clwb(&p_items);
    PMEM::sfence();
    free(old);
  }

public:
  /// Construct an empty P_UndoLog with a default capacity.  We must take care
  /// to ensure that the timestamp is set, the backing storage is zeroed, and
  /// both are persisted.
  P_UndoLog(const unsigned long _capacity = 64)
      : capacity(_capacity), count(0), p_items(nullptr), p_timestamp(1) {
    // flush the timestamp immediately.  We will assume that items==nullptr
    // flushes with timestamp, and thus recovery will know there's nothing to
    // recover
    PMEM::clwb(&p_timestamp);
    // Get memory, zero it, flush it
    char *items_array =
        (char *)aligned_alloc(CACHELINE_BYTES, sizeof(p_logentry_t) * capacity);
    memset(items_array, 0, sizeof(p_logentry_t) * capacity);
    for (size_t i = 0; i < sizeof(p_logentry_t) * capacity;
         i += CACHELINE_BYTES) {
      PMEM::clwb(items_array + i);
    }
    PMEM::sfence();
    p_items = (p_logentry_t *)items_array;
    PMEM::clwb(&p_items);
    PMEM::sfence();
  }

  /// Reclaim memory when the UndoLog is destructed
  ~P_UndoLog() { free(p_items); }

  /// We assume that T does not have a destructor, and thus we can fast-clear
  /// the UndoLog
  void p_clear() {
    count = 0;
    // Update and persist the timestamp, so that subsequent recovery knows to
    // ignore all entries
    ++p_timestamp;
    PMEM::clwb(&p_timestamp);
  }

  /// Return whether the UndoLog is empty or not
  bool empty() { return count == 0; }

  /// UndoLog insert
  ///
  /// We maintain the invariant that when insert() returns, there is always room
  /// for one more element to be added.  This means we may expand() after
  /// insertion, but doing so is rare.
  template <typename T> void p_push_back(T *ptr) {
    p_items[count++].p_initFromAddr(ptr, p_timestamp);

    // If the list is full, double it
    if (count == capacity)
      p_expand();
  }

  /// Getter to report the array size (to test for empty)
  unsigned long size() const { return count; }

  /// UndoLog's iterator is just a T*
  typedef p_logentry_t *iterator;

  /// Get an iterator to the start of the array
  iterator begin() const { return p_items; }

  /// Get an iterator to one past the end of the array
  iterator end() const { return p_items + count; }

  /// UndoLog's reverse iteration is based on std::reverse_iterator, because
  /// this is not performance-critical
  typedef std::reverse_iterator<iterator> reverse_iterator;

  /// Get the starting point for a reverse iterator
  reverse_iterator rbegin() { return reverse_iterator(end()); }

  /// Get the ending point for a reverse iterator
  reverse_iterator rend() { return reverse_iterator(begin()); }
};
