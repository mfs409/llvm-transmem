#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>

#include "platform.h"

/// P_MiniVector is a self-growing array, like std::vector, but with less
/// overhead.  It differs from MiniVector by being persistent.
///
/// Since we never remove single elements from this datastructure, the only
/// persistence issues we face are when clearing and when inserting an element.
/// We use the same approach as in p_undolog.h.  That is, we have both a counter
/// and a timestamp, and we put the timestamp into each vector entry.  To
/// fast-clear, we update the timestamp persistently (flush and fence).
///
/// NB: PMEM is for switching between ADR and eADR
template <class T, class PMEM> class P_MiniVector {

  /// The type we store in the vector
  struct p_entry_t {
    /// val is the actual, useful data
    T val;

    /// A timestamp, for easier persistence
    uint64_t timestamp;

    /// Pad out to 32 bytes, so that we can be sure to only need one clwb per
    /// store.  Note that we expect sizeof(T) >= sizeof(uint64_t).
    char pad[32 - sizeof(T) - sizeof(uint64_t)];
  };

  /// The maximum number of things this MiniVector can store without triggering
  /// a resize
  unsigned long capacity;

  /// The number of items currently stored in the MiniVector
  unsigned long count;

  /// Storage for the items in the array (persistent)
  p_entry_t *p_items;

  /// Timestamp, for tracking invalid entries (persistent)
  uint64_t p_timestamp;

  /// Resize the array of items, and move current data into it
  void p_expand() {
    capacity *= 2;
    char *temp =
        (char *)aligned_alloc(CACHELINE_BYTES, sizeof(p_entry_t) * capacity);
    memset(temp, 0, sizeof(p_entry_t) * capacity);
    // Copy everything over, persist it
    memcpy(temp, p_items, sizeof(p_entry_t) * count);
    for (unsigned long i = 0; i < capacity; i += CACHELINE_BYTES) {
      PMEM::clwb(temp + i);
    }
    PMEM::sfence();
    // install pointer to new data, delete old data
    p_entry_t *old = p_items;
    p_items = (p_entry_t *)temp;
    PMEM::clwb(&p_items);
    PMEM::sfence();
    free(old);
  }

public:
  /// Construct an empty MiniVector with a default capacity
  P_MiniVector(const unsigned long _capacity = 64)
      : capacity(_capacity), count(0), p_items(nullptr), p_timestamp(1) {
    // flush the timestamp immediately.  We will assume that items==nullptr
    // flushes with timestamp, and thus recovery will know there's nothing to
    // recover
    PMEM::clwb(&p_timestamp);
    // Get memory, zero it, flush it
    char *items_array =
        (char *)aligned_alloc(CACHELINE_BYTES, sizeof(p_entry_t) * capacity);
    memset(items_array, 0, sizeof(p_entry_t) * capacity);
    for (size_t i = 0; i < sizeof(p_entry_t) * capacity; i += CACHELINE_BYTES) {
      PMEM::clwb(items_array + i);
    }
    PMEM::sfence();
    p_items = (p_entry_t *)items_array;
    PMEM::clwb(&p_items);
    PMEM::sfence();
  }

  /// Reclaim memory when the MiniVector is destructed
  ~P_MiniVector() { free(p_items); }

  /// We assume that T does not have a destructor, and thus we can fast-clear
  /// the MiniVector
  void p_clear() {
    count = 0;
    // Update and persist the timestamp, so that subsequent recovery knows to
    // ignore all entries
    ++p_timestamp;
    PMEM::clwb(&p_timestamp);
    PMEM::sfence();
  }

  /// Return whether the MiniVector is empty or not
  bool empty() { return count == 0; }

  /// MiniVector insert
  ///
  /// We maintain the invariant that when insert() returns, there is always room
  /// for one more element to be added.  This means we may expand() after
  /// insertion, but doing so is rare.
  void push_back(T data) {
    p_items[count].val = data;
    p_items[count].timestamp = p_timestamp;
    PMEM::clwb(&p_items[count]);
    PMEM::sfence();
    ++count;

    // If the list is full, double it
    if (count == capacity)
      p_expand();
  }

  /// Getter to report the array size (to test for empty)
  unsigned long size() const { return count; }

  /// P_MiniVector's iterator is the entry type
  typedef p_entry_t *iterator;

  /// Get an iterator to the start of the array
  iterator begin() const { return p_items; }

  /// Get an iterator to one past the end of the array
  iterator end() const { return p_items + count; }
};