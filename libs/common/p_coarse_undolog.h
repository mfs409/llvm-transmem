#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string.h>

#include "pmem_platform.h"

/// P_CoarseUndoLog is a variant of P_RedoLog in which all logging is done at a
/// coarse granularity, and in which the usage is undo logging (hence this
/// object reads from memory rather than having values provided to it).  Be sure
/// to see P_RedoLog for details about the implementation.
///
/// NB: CHUNKSIZE must be <= 64, and a power of 2.  It should be at least as
///     large as the largest scalar datatype supported by the language.  16, 32,
///     and 64 are probably the only reasonable values.
template <int CHUNKSIZE> class P_CoarseUndoLog {
  /// MASK is used to isolate/clear the low bits of an address. It is dependent
  /// on CHUNKSIZE
  static const uintptr_t MASK = ((uintptr_t)CHUNKSIZE) - 1;

  /// The hash index consists of a version (for fast clearing), an address, and
  /// an index into the vector.
  struct index_t {
    /// Version number... if it doesn't match the UndoLog's version number, this
    /// index_t is not in use
    size_t version;

    /// The key is an address
    uintptr_t address;

    /// The value is an index into the vector
    size_t index;

    /// Constructor: Note that version 0 is not allowed
    index_t() : version(0), address(0), index(0) {}
  };

  /// The vector holds these: they correspond to a range of bytes that may be
  /// written back.  We force them to be an exact number of cache lines in size,
  /// noting that they'll only ever be one or two.
  struct undo_chunk_t {
    /// The base address
    uintptr_t key;

    /// The actual data
    uint8_t data[CHUNKSIZE];

    /// Restore the chunk to memory
    void p_restoreValue() {
      memcpy((void *)key, (void *)data, CHUNKSIZE);
      pmem_clwb((void *)key);
    }

    /// Pad it out to a fixed number of cachelines:
    char pad[(((CHUNKSIZE + sizeof(uintptr_t)) <= CACHELINE_BYTES)
                  ? CACHELINE_BYTES
                  : 2 * CACHELINE_BYTES) -
             sizeof(uintptr_t) - CHUNKSIZE];
  };

  /// number of static probes before we resize the list
  static const int SPILL_FACTOR = 3;

  /// The "hashtable" of the Redo Log
  index_t *index;

  /// Size of hashtable
  size_t ilength;

  /// For fast-clearing of the hash table
  size_t version;

  /// used by the hash function
  size_t shift;

  /// The "vector" of the Redo Log.  When we (re) allocate it, we align it on a
  /// cache line boundary.
  undo_chunk_t *undo_chunks;

  /// Capacity of the vector
  size_t vector_capacity;

  /// Current # elements in vector
  size_t vector_size;

  /// This hash function is straight from CLRS (that's where the magic constant
  /// comes from).
  size_t hash(uintptr_t const key) const {
    static const unsigned long long s = 2654435769ull;
    const unsigned long long r = ((unsigned long long)key) * s;
    return (size_t)((r & 0xFFFFFFFF) >> shift);
  }

  /// Double the size of the index. This *does not* do anything as far as
  /// actually doing memory allocation. Callers should delete[] the index table,
  /// increment the table size, and then reallocate it.
  size_t doubleIndexLength() {
    assert(shift != 0 &&
           "ERROR: the writeset doesn't support an index this large");
    shift -= 1;
    ilength = 1 << (8 * sizeof(uint32_t) - shift);
    return ilength;
  }

  /// Increase the size of the hash and rehash everything
  __attribute__((noinline)) void rebuild() {
    assert(version != 0 && "ERROR: the version should *never* be 0");

    // double the index size
    delete[] index;
    index = new index_t[doubleIndexLength()];

    // rehash the elements
    for (size_t i = 0; i < vector_size; ++i) {
      size_t h = hash(undo_chunks[i].key);

      // search for the next available slot
      while (index[h].version == version)
        h = (h + 1) % ilength;

      index[h].address = undo_chunks[i].key;
      index[h].version = version;
      index[h].index = i;
    }
  }

  /// Double the size of the vector if/when it becomes full
  __attribute__((noinline)) void resize() {
    // create space for new vector
    vector_capacity *= 2;
    char *temp = (char *)aligned_alloc(CACHELINE_BYTES,
                                       vector_capacity * sizeof(undo_chunk_t));
    // copy everything over, and persist it
    memcpy(temp, undo_chunks, sizeof(undo_chunk_t) * vector_size);
    for (int i = 0; i < sizeof(undo_chunk_t) * vector_size;
         i += CACHELINE_BYTES) {
      pmem_clwb(temp + i);
    }
    // install pointer to new data, delete old data
    undo_chunk_t *old = undo_chunks;
    undo_chunks = (undo_chunk_t *)temp;
    free(old);
    // be sure to persist the pointer to the vector, so it's visible at recovery
    // time
    pmem_clwb(&undo_chunks);
  }

  /// zero the hash on version# overflow... highly unlikely
  __attribute__((noinline)) void reset_internal() {
    memset(index, 0, sizeof(index_t) * ilength);
    version = 1;
  }

public:
  /// Construct an UndoLog by providing an initial capacity (default 64)
  P_CoarseUndoLog(const size_t initial_capacity = 64)
      : index(nullptr), ilength(0), version(1), shift(8 * sizeof(uint32_t)),
        undo_chunks(nullptr), vector_capacity(initial_capacity),
        vector_size(0) {
    // Find a good index length for the initial capacity of the list.
    while (ilength < SPILL_FACTOR * initial_capacity)
      doubleIndexLength();
    index = new index_t[ilength];
    undo_chunks = (undo_chunk_t *)aligned_alloc(
        CACHELINE_BYTES, vector_capacity * sizeof(undo_chunk_t));
    // make sure the pointer to the data and its size are persisted before
    // we return
    pmem_clwb(&undo_chunks);
    pmem_clwb(&vector_size);
    pmem_sfence();
  }

  /// Reclaim the dynamically allocated parts of an UndoLog when we destroy it
  ~P_CoarseUndoLog() {
    delete[] index;
    free(undo_chunks);
  }

  /// Fast check if the undo log is empty
  bool empty() const { return vector_size == 0; }

  /// Return the size of the undo log
  int size() const { return vector_size; }

  /// fast-clear the hash by bumping the version number
  void p_clear() {
    vector_size = 0;
    version += 1;
    pmem_clwb(&vector_size); // persist it
    // check overflow
    if (version != 0)
      return;
    reset_internal();
  }

  /// Pushing an address into the undo log is supposed to log the address/value
  /// pair given by dereferencing the given address.  However, this is a
  /// hash-based log, so what we really do is check if the log has this chunk
  /// already.  If so, do nothing.  If not, get the whole chunk, log it, and
  /// persist the log entry.
  template <typename T> void p_push_back(T *addr) {
    // Figure out the aligned block of memory to log
    uintptr_t key = (uintptr_t)addr & ~MASK;

    // Find the slot that this address should hash to. If it is valid, we
    // already logged it.  Otherwise, we will need to create a new log entry.
    size_t h = hash(key);
    while (index[h].version == version) {
      if (index[h].address == key) {
        return;
      }
      // keep probing...
      h = (h + 1) % ilength;
    }

    // at this point, h is a valid insertion point
    index[h].address = key;
    index[h].version = version;
    index[h].index = vector_size;

    // initialize the vector's key and data
    undo_chunks[vector_size].key = key;
    memcpy(undo_chunks[vector_size].data, (void *)key, CHUNKSIZE);

    // flush it, making sure not to miss any bytes of the entry
    for (int i = 0; i < sizeof(undo_chunk_t); i += CACHELINE_BYTES) {
      pmem_clwb(((char *)&undo_chunks[vector_size]) + i);
    }

    // update the next element pointer into the vector
    ++vector_size;
    pmem_clwb(&vector_size);

    // resize the vector if there's only one spot left
    if (__builtin_expect(vector_size == vector_capacity, false)) {
      resize();
    }

    // if we reach our load-factor, rebuild
    if (__builtin_expect((vector_size * SPILL_FACTOR) >= ilength, false)) {
      rebuild();
    }
  }

  /// The iterator is just a pointer to a undo chunk
  typedef undo_chunk_t *iterator;

  /// Get an iterator to the start of the array.  Note that the use case for
  /// this undolog thinks it needs reverse iterators, because it assumes a
  /// time-ordered log.  Since this is a hashing log, we can just go forward,
  /// but we call it rbegin for compatibility.
  iterator rbegin() const { return undo_chunks; }

  /// Get an iterator to one past the end of the array
  iterator rend() const { return undo_chunks + vector_size; }
};
