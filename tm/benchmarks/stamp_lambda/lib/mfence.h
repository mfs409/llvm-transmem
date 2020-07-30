/* Copyright (c) IBM Corp. 2014. */

#if defined(__x86_64__)
#include <emmintrin.h>
#endif
/* requires C99 */
static inline void memory_fence(void) {
#if defined(__370__)
    /* Nothing to do */
#elif defined(__PPC__) && defined(__IBMC__)
  __lwsync();
#elif defined(__PPC__) || defined(_ARCH_PPC)
    asm volatile("lwsync" : : );
#elif defined(__x86_64__)
  _mm_sfence();
#elif defined(__GNUC__) || defined(__IBMC__)
    __sync_synchronize();
#else
#error
#endif
}

