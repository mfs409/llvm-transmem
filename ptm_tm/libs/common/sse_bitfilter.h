#pragma once

#include <stdint.h>
#include <xmmintrin.h>

/// SSEBitFilter is an SSE-accelerated version of BitFilter.  See bitfilter.h
/// for additional details.
template <uint32_t BITS, int GRAIN> class SSEBitFilter {
  /// The number of bits in an SSE register
  static const uint32_t VEC_SIZE = 8 * sizeof(__m128i);

  /// The number of SSE registers needed to get the desired number of bits
  static const uint32_t VEC_BLOCKS = BITS / VEC_SIZE;

  /// The number of bits in a word
  static const uint32_t WORD_SIZE = 8 * sizeof(uintptr_t);

  /// The number of words needed to get the desired number of bits
  static const uint32_t WORD_BLOCKS = BITS / WORD_SIZE;

  /// The contiguous region of bits that represents the set
  union {
    mutable __m128i vec_filter[VEC_BLOCKS];
    uintptr_t word_filter[WORD_BLOCKS];
  } __attribute__((aligned(16)));

  /// Given a pointer, compute the corresponding bit position in word_filter
  static uint32_t hash(const void *const key) {
    return (((uintptr_t)key) >> GRAIN) % BITS;
  }

public:
  /// Allocate a filter that is aligned properly for use with SSE
  static void *filter_alloc(size_t s) { return _mm_malloc(s, 16); }

  /// Construct an SSEBitFilter by ensuring that it is clear
  SSEBitFilter() { clear(); }

  /// Set a bit in the filter, based on a provided pointer
  void add(const void *const val) volatile {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    word_filter[block] |= (1u << offset);
  }

  /// Set a bit in the filter, based on a provided pointer, but do so with
  /// strong memory ordering.
  void atomic_add(const void *const val) volatile {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    atomicswapptr(&word_filter[block], word_filter[block] | (1u << offset));
  }

  /// Test if the bit corresponding to a pointer is set
  bool lookup(const void *const val) const volatile {
    const uint32_t index = hash(val);
    const uint32_t block = index / WORD_SIZE;
    const uint32_t offset = index % WORD_SIZE;
    return word_filter[block] & (1u << offset);
  }

  /// Modify the SSEBitFilter by unioning it with the provided SSEBitFilter
  void unionwith(const SSEBitFilter<BITS, GRAIN> &rhs) {
    for (uint32_t i = 0; i < VEC_BLOCKS; ++i) {
      vec_filter[i] = _mm_or_si128(vec_filter[i], rhs.vec_filter[i]);
    }
  }

  /// Clear an SSEBitFilter
  void clear() volatile {
    // NB: This loop gets automatically unrolled for BITS = 1024 by gcc-4.3.3
    const __m128i zero = _mm_setzero_si128();
    for (uint32_t i = 0; i < VEC_BLOCKS; ++i) {
      vec_filter[i] = zero;
    }
  }

  /// Set this SSEBitFilter to have the same contents as the provided
  /// SSEBitFilter
  void fastcopy(const SSEBitFilter<BITS, GRAIN> *rhs) volatile {
    for (uint32_t i = 0; i < VEC_BLOCKS; ++i) {
      vec_filter[i] = rhs->vec_filter[i];
    }
  }

  /// Intersect the current SSEBitFilter with the provided SSEBitFilter.  Return
  /// true if the result is nonzero.
  bool intersect(const SSEBitFilter<BITS, GRAIN> *rhs) const volatile {
    // There is no clean way to compare an __m128i to zero, so we have
    // to union it with an array of uint64_ts, and then we can look at
    // the vector 64 bits at a time
    union {
      __m128i v;
      uint64_t i[2];
    } tmp __attribute__((aligned(16)));
    tmp.i[0] = tmp.i[1] = 0;
    for (uint32_t i = 0; i < VEC_BLOCKS; ++i) {
      __m128i intersect = _mm_and_si128(vec_filter[i], rhs->vec_filter[i]);
      tmp.v = _mm_or_si128(tmp.v, intersect);
    }
    return tmp.i[0] | tmp.i[1];
  }
};