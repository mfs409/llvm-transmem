#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string.h>

/// P_RedoLog is a PTM-friendly version of the STM RedoLog object.  It combines
/// a hash-based index with a vector, so that we can quickly find elements, and
/// still iterate through the collection efficiently.  Within the vector, we
/// store chunks of contiguous bytes, so that it is easy to re-assemble data
/// that was written at varying granularities.
///
/// In P_RedoLog, we do not need to persist the index, only the vector. This
/// requires care on vector resize.  It might seem that we also need to persist
/// the size of the vector before we can commit, but we don't, because we can
/// infer the vector size by looking for a vector entry with key==0.
///
/// NB: CHUNKSIZE must be <= 64, and a power of 2.  It should be at least as
///     large as the largest scalar datatype supported by the language.  16, 32,
///     and 64 are probably the only reasonable values.
///
/// NB: CHUNKSIZE dictates the alignment that the compiler must obey.  That is,
///     when it is 8, a scalar variable cannot cross an 8-byte boundary, or we
///     will not be able to log it correctly.  On SPARC, we'd get a bus error
///     anyway.  But on x86, such mis-alignment is possible.
///
/// NB: There are two orthogonal issues related to the timing of clwb
///     instructions in a persistent redolog.  The first is "when to persist the
///     log".  The log must be persisted before the transaction marks itself as
///     committed, or else doing redo during recovery may fail.  The two ways to
///     handle this are (1) issue a clwb whenever a redo log entry is updated,
///     or (2) issue a bunch of clwbs right before setting the state to
///     committed.  The second issue is "when do writebacks get clwb'd".  The
///     issue here is that we may want to avoid doing clwb instructions while
///     holding locks, even though it complicates recovery.  The
///     EARLY_PERSIST_ENTRY and EARLY_PERSIST_WRITEBACK parameters govern how
///     these two behaviors are handled.
///
/// NB: PMEM is for switching between ADR and eADR
template <int CHUNKSIZE, bool EARLY_PERSIST_ENTRY, bool EARLY_PERSIST_WRITEBACK,
          class PMEM>
class P_RedoLog {
public:
  /// Report the granularity at which this redo log operates, as a number of
  /// 64-bit words
  static const int REDO_WORDS = CHUNKSIZE / sizeof(uint64_t);

  /// RedoLog Chunk Size
  static const int REDO_CHUNKSIZE = CHUNKSIZE;

  /// The template determines if threads using this RedoLog need to use
  /// timestamps for their status, or if a simple active/inactive flag suffices
  static const bool WRITEBACK_NEEDS_TIMESTAMP = !EARLY_PERSIST_WRITEBACK;

  /// find() operations will return a mask that the redo log uses to determine
  /// the validity of the result.  This is its type.
  typedef uint32_t mask_t;

  /// For a given type's size, create the kind of mask that this redolog wishes
  /// to use for reconstruction
  static mask_t create_mask(size_t typesize) { return (1UL << typesize) - 1; }

private:
  /// MASK is used to isolate/clear the low bits of an address. It is dependent
  /// on CHUNKSIZE
  static const uintptr_t MASK = ((uintptr_t)CHUNKSIZE) - 1;

  /// The hash index consists of a version (for fast clearing), an address, and
  /// an index into the vector.
  struct index_t {
    /// Version number... if it doesn't match the RedoLog's version number, this
    /// index_t is not in use
    size_t version;

    /// The key is an address
    uintptr_t address;

    /// The value is an index into the vector
    size_t index;

    /// Constructor: Note that version 0 is not allowed in the RedoLog
    index_t() : version(0), address(0), index(0) {}
  };

  /// The vector holds these: they correspond to a range of bytes that may be
  /// written back.
  struct writeback_chunk_t {
    /// The actual data
    uint8_t data[CHUNKSIZE];

    /// The base address
    uintptr_t key;

    /// Bitmask for which bytes are valid
    uint64_t mask;

    /// Pad it out to a fixed number of cachelines:
    char pad[(((CHUNKSIZE + sizeof(uintptr_t) + sizeof(uint64_t)) <=
               CACHELINE_BYTES)
                  ? CACHELINE_BYTES
                  : 2 * CACHELINE_BYTES) -
             sizeof(uintptr_t) - sizeof(uint64_t) - CHUNKSIZE];
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

  /// The "vector" of the Redo Log
  writeback_chunk_t *redo_vector;

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
      size_t h = hash(redo_vector[i].key);

      // search for the next available slot
      while (index[h].version == version)
        h = (h + 1) % ilength;

      index[h].address = redo_vector[i].key;
      index[h].version = version;
      index[h].index = i;
    }
  }

  /// Double the size of the vector if/when it becomes full
  __attribute__((noinline)) void resize() {
    // create space for new vector
    vector_capacity *= 2;
    char *temp = (char *)aligned_alloc(
        CACHELINE_BYTES, vector_capacity * sizeof(writeback_chunk_t));
    // copy everything over, and possibly persist it
    memcpy(temp, redo_vector, sizeof(writeback_chunk_t) * vector_size);
    if (EARLY_PERSIST_ENTRY) {
      for (size_t i = 0; i < sizeof(writeback_chunk_t) * vector_size;
           i += CACHELINE_BYTES) {
        PMEM::clwb(temp + i);
      }
    }
    // install pointer to new data, delete old data
    writeback_chunk_t *old = redo_vector;
    redo_vector = (writeback_chunk_t *)temp;
    free(old);
    // be sure to persist the pointer to the vector, so it's visible at recovery
    // time
    PMEM::clwb(&redo_vector);
  }

  /// zero the hash on version# overflow... highly unlikely
  __attribute__((noinline)) void reset_internal() {
    memset(index, 0, sizeof(index_t) * ilength);
    version = 1;
  }

  /// reserve is effectively the first half of an "upsert".  It finds the vector
  /// entry into which a key should go, or makes that vector entry
  ///
  /// NB: we expect key's low bits to be masked to zero
  int reserve(uintptr_t key) {
    //  Find the slot that this address should hash to. If it is valid,
    //  return the index. If we find an unused slot then it's a new
    //  insertion.
    size_t h = hash(key);
    while (index[h].version == version) {
      if (index[h].address == key) {
        return index[h].index;
      }
      // keep probing...
      h = (h + 1) % ilength;
    }

    // at this point, h is a valid insertion point
    index[h].address = key;
    index[h].version = version;
    index[h].index = vector_size;

    // initialize the vector
    redo_vector[vector_size].key = key;
    redo_vector[vector_size].mask = 0LL;

    // update the next element pointer into the vector
    ++vector_size;

    // resize the vector if there's only one spot left
    if (__builtin_expect(vector_size == vector_capacity, false))
      resize();

    // if we reach our load-factor, rebuild
    if (__builtin_expect((vector_size * SPILL_FACTOR) >= ilength, false)) {
      rebuild();
      return reserve(key);
    }

    return index[h].index;
  }

  /// Find the vector index of the chunk containing key, or -1 on failure
  int lookup(uintptr_t key) {
    size_t h = hash(key);
    while (index[h].version == version) {
      if (index[h].address != key) {
        // use linear probing... given SPILL_FACTOR, we never wrap around
        h = (h + 1) % ilength;
        continue;
      }
      return index[h].index;
    }
    return -1;
  }

public:
  /// Construct a RedoLog by providing an initial capacity (default 64)
  P_RedoLog(const size_t initial_capacity = 64)
      : index(nullptr), ilength(0), version(1), shift(8 * sizeof(uint32_t)),
        redo_vector(nullptr), vector_capacity(initial_capacity),
        vector_size(0) {
    // Find a good index length for the initial capacity of the list.
    while (ilength < SPILL_FACTOR * initial_capacity)
      doubleIndexLength();
    index = new index_t[ilength];
    redo_vector = (writeback_chunk_t *)aligned_alloc(
        CACHELINE_BYTES, vector_capacity * sizeof(writeback_chunk_t));
    // make sure the pointer to the redo log and its size are persisted before
    // we return
    PMEM::clwb(&redo_vector);
    PMEM::clwb(&vector_size);
    PMEM::sfence();
  }

  /// Reclaim the dynamically allocated parts of a RedoLog when we destroy it
  ~P_RedoLog() {
    delete[] index;
    free(redo_vector);
  }

  /// Fast check if the RedoLog is empty
  bool isEmpty() const { return vector_size == 0; }

  /// fast-clear the hash by bumping the version number
  void clear() {
    vector_size = 0;
    version += 1;
    // check overflow
    if (version != 0)
      return;
    reset_internal();
  }

  /// write the contents of the redolog back to main memory.
  void p_writeback() {
    // iterate through the slabs, and then write out the bytes
    for (size_t i = 0; i < vector_size; ++i) {
      for (int bytes = 0; bytes < CHUNKSIZE; bytes += 4) {
        // figure out if current 4 bytes are all valid
        int m = redo_vector[i].mask >> bytes;
        m = m & 0xF;
        if (m == 0xF) {
          // we can write this as a 32-bit word
          uint32_t *addr = (uint32_t *)(redo_vector[i].key + bytes);
          uint32_t *data = (uint32_t *)(redo_vector[i].data + bytes);
          *addr = *data;
        } else if (m != 0) {
          // write out live bytes, one at a time
          uint8_t *addr = (uint8_t *)redo_vector[i].key + bytes;
          uint8_t *data = redo_vector[i].data + bytes;
          // NB: an easily-unrolled loop probably outperforms a while loop
          for (int q = 0; q < 4; ++q) {
            if (m & 1) {
              *addr = *data;
              // NB: redolog_pmem had this:
              // asm_flush(addr);
            }
            ++addr;
            ++data;
            m >>= 1;
          }
        }
      }
      // If we are persisting early, then flush the cache line that we just
      // wrote
      if (EARLY_PERSIST_WRITEBACK) {
        PMEM::clwb((void *)redo_vector[i].key);
      }
    }
  }

  /// If we didn't clwb the writes to main memory during writeback, clwb them
  /// now
  void p_postcommit() {
    if (EARLY_PERSIST_WRITEBACK) {
      return;
    }
    // iterate through the slabs, and then write out the bytes
    for (size_t i = 0; i < vector_size; ++i) {
      PMEM::clwb((void *)redo_vector[i].key);
    }
  }

  /// Before committing, we must be sure that the entire redo log vector is
  /// flushed, and also that the size of the vector is flushed.
  void p_precommit() {
    // NB: we don't need to flush the vector contents if we have been flushing
    //     them along the way.
    if (!EARLY_PERSIST_ENTRY) {
      char *temp = (char *)redo_vector;
      for (size_t i = 0; i < sizeof(writeback_chunk_t) * vector_size;
           i += CACHELINE_BYTES) {
        PMEM::clwb(temp + i);
      }
    }
    // But we always need to flush the vector size, because we don't flush it on
    // each insertion.
    PMEM::clwb(&vector_size);
  }

  /// type-specialized code for inserting an address/value pair into the RedoLog
  template <typename T> bool insert_if(T *addr, T val) {
    // compute mask, key, and offset within block
    uint64_t mask = (1UL << sizeof(T)) - 1;
    uintptr_t key = (uintptr_t)addr & ~MASK;
    uint64_t offset = (uintptr_t)addr & MASK;

    // get slab index, transform into in-slab address where we should write
    int idx = reserve(key);
    uint8_t *dataptr = redo_vector[idx].data;
    dataptr += offset;

    // do a type-preserving write, and update the mask
    T *tgt = (T *)dataptr;
    *tgt = val;
    redo_vector[idx].mask |= (mask << offset);
    // Possibly persist the redo_vector entry that we just updated
    if (EARLY_PERSIST_ENTRY) {
      // flush it, making sure not to miss any bytes of the entry
      for (size_t i = 0; i < sizeof(writeback_chunk_t); i += CACHELINE_BYTES) {
        PMEM::clwb(((char *)&redo_vector[idx]) + i);
      }
    }
    return true;
  }

  /// in this redolog implementation, insert_with() is invalid
  template <typename T> void insert_with(T *, T, uint64_t *) {
    std::terminate();
  }

  /// type-specialized code for looking up a value from the RedoLog
  template <typename T> mask_t get_partial(T *addr, T &val) {
    /// compute mask, key, and offset within block
    uint64_t mask = (1UL << sizeof(T)) - 1;
    uintptr_t key = (uintptr_t)addr & ~MASK;
    uint64_t offset = (uintptr_t)addr & MASK;

    // get slab target, see if it's valid
    int idx = lookup(key);
    if (idx == -1)
      return 0; // not valid because it doesn't exist
    uint64_t nodemask = redo_vector[idx].mask >> offset;
    mask_t livebits = mask & nodemask;
    if (!livebits)
      return 0; // not valid because offset all 0s in mask
    // It's valid, so read a full T's worth of data
    uint8_t *dataptr = redo_vector[idx].data;
    dataptr += offset;
    T *tgt = (T *)dataptr;
    val = *tgt;
    // return the mask, since some of the bytes we just read may be invalid
    return livebits;
  }

  /// Sometimes, a program will have different granularities (byte, int, long)
  /// when accessing the same region of memory at different times, in the same
  /// transaction.  If a smaller-granularity write is followed by a
  /// larger-granularity read, then find() could result in a partial hit (found
  /// mask is some ones, but not sizeof(T) ones).  In that case, we must copy
  /// the bytes from the redo log *onto* the bytes previously from memory, one
  /// byte at a time, using the mask.  This function does the job, with
  /// pass-by-reference.
  template <typename T>
  static void reconstruct(T &from_mem, T &ret, mask_t &found_mask) {
    // This is the unlikely, slow path.  We must weave the bytes together in
    // accordance with the mask.  It is essentially a memcpy, one byte at a
    // time, only for bytes whose corresponding bit in the mask is 1.  Hopefully
    // this unrolls nicely
    char *from = (char *)&from_mem;
    char *to = (char *)&ret;
    int working_mask = 1;
    for (size_t i = 0; i < sizeof(T); ++i) {
      if (!(found_mask & working_mask)) {
        *to = *from;
      }
      ++from;
      ++to;
      working_mask <<= 1;
    }
  }
};
