/// mem.h implements the portion of the API required for malloc/free and
/// memcpy/memset/memmove

#pragma once

#include "../common/memfuncs.h"

/// Create the API functions that are substituted for transactional calls to
/// malloc(), aligned_malloc() and free().
#define API_TM_MALLOC_FREE                                                     \
  extern "C" {                                                                 \
  void *TM_MALLOC(size_t size) { return get_self()->txAlloc(size); }           \
  void *TM_ALIGNED_ALLOC(size_t align, size_t size) {                          \
    return get_self()->txAAlloc(align, size);                                  \
  }                                                                            \
  void TM_FREE(void *ptr) { get_self()->txFree(ptr); }                         \
  }

/// Create the API functions that are substituted for transactional calls to
/// memcpy, memset, and memmove.  These are the generic versions, which do
/// nothing special to optimize instrumentation (i.e., no bulk undo logging,
/// bulk ownership acquisition, etc.).
///
/// NB: By way of a transaction's "irrevocable" state, these can simply forward
/// to the corresponding C library functions
#define API_TM_MEMFUNCS_GENERIC                                                \
  extern "C" {                                                                 \
  void *TM_MEMCPY(void *dest, const void *src, size_t count, size_t align) {   \
    return tx_basic_memcpy(dest, src, count, align, *get_self());              \
  }                                                                            \
  void *TM_MEMSET(void *dest, int ch, size_t count) {                          \
    return tx_basic_memset(dest, ch, count, *get_self());                      \
  }                                                                            \
  void *TM_MEMMOVE(void *dest, const void *src, size_t count) {                \
    return tx_basic_memmove(dest, src, count, *get_self());                    \
  }                                                                            \
  }