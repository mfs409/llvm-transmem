#pragma once

#include <stdint.h>

/// BitFilter is an imprecise set representation, in which elements of the
/// universe (all addresses) are many-to-one mapped to bits in a dense bit
/// vector.
///
/// BitFilter supports insert, remove, union, and intersection, as well as an
/// atomic insert operation.
///
/// - BITS indicates the number of bits in the BitFilter.  It should be a power
///   of 2, but the code does no enforcement.
/// - GRAIN indicates the granularity at which addresses are mapped to bits
///   (e.g., the number of low-order bits that are ignored)
///
/// NB: There is also SSE_BitFilter.  However, it needs includes that may not be
///     available on some architectures, so we put it in another file.
template <uint32_t BITS, int GRAIN> class BitFilter {
  /// The number of bits in a word
  static const uint32_t WORD_SIZE = 8 * sizeof(uintptr_t);

  /// The number of words needed to get the desired number of bits
  static const uint32_t WORD_BLOCKS = BITS / WORD_SIZE;

  /// The contiguous region of bits that represents the set
  uintptr_t word_filter[WORD_BLOCKS] __attribute__((aligned(16)));

  /// Given a pointer, compute the corresponding bit position in word_filter
  static uint32_t hash(const void *const key) {
    return (((uintptr_t)key) >> GRAIN) % BITS;
  }

public:
  /// Allocate a filter
  static void *filter_alloc(size_t s) { return malloc(s); }

  /// Construct a BitFilter by ensuring that it is clear
  BitFilter() { clear(); }

  /// Set a bit in the filter, based on a provided pointer
  void add(const void *const val) {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    word_filter[block] |= (1u << offset);
  }

  /// Set a bit in the filter, based on a provided pointer, but do so with
  /// strong memory ordering.
  void atomic_add(const void *const val) {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    atomicswapptr(&word_filter[block], word_filter[block] | (1u << offset));
  }

  /// Test if the bit corresponding to a pointer is set
  bool lookup(const void *const val) const {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    return word_filter[block] & (1u << offset);
  }

  /// Modify the BitFilter by unioning it with the provided BitFilter
  void unionwith(const BitFilter<BITS, GRAIN> &rhs) {
    for (uint32_t i = 0; i < WORD_BLOCKS; ++i) {
      word_filter[i] |= rhs.word_filter[i];
    }
  }

  /// Clear a BitFilter
  void clear() {
    for (uint32_t i = 0; i < WORD_BLOCKS; ++i) {
      word_filter[i] = 0;
    }
  }

  /// Set this BitFilter to have the same contents as the provided BitFilter
  void fastcopy(const BitFilter<BITS, GRAIN> *rhs) {
    for (uint32_t i = 0; i < WORD_BLOCKS; ++i) {
      word_filter[i] = rhs->word_filter[i];
    }
  }

  /// Intersect the current BitFilter with the provided BitFilter.  Return true
  /// if the result is nonzero.
  bool intersect(const BitFilter<BITS, GRAIN> *rhs) const {
    bool res = false;
    for (uint32_t i = 0; i < WORD_BLOCKS; ++i) {
      res |= (word_filter[i] & rhs->word_filter[i]);
    }
    return res;
  }
};