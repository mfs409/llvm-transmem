/// pmem_platform.h provides platform-specific pmem assembly code

#pragma once

#include <xmmintrin.h>

/// pmem_clwb represents the cache-line writeback instruction
void pmem_clwb(void *addr) {
  asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
}

/// pmem_sfence() is a thin wrapper around the sfence intrinsic
void pmem_sfence() { _mm_sfence(); }