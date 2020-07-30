/// pmem_platform.h provides platform-specific pmem assembly code

#pragma once

#include <xmmintrin.h>

/// pmem_adr implements flushes (clwb) and fences (sfence) appropriately for an
/// ADR system.
class pmem_adr {
public:
  /// pmem_clwb represents the cache-line writeback instruction
  static void clwb(void *addr) {
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
  }

  /// pmem_sfence() is a thin wrapper around the sfence intrinsic
  static void sfence() { _mm_sfence(); }
};

/// pmem_dram does not implements flushes (clwb), but does implement and fences
/// (sfence).  It could be thought of as eADR, or as being a way to elide CLWB
/// to see its cost.
class pmem_dram {
public:
  /// pmem_clwb is a no-op in pmem_dram
  static void clwb(void *) {}

  /// pmem_sfence() is a thin wrapper around the sfence intrinsic
  static void sfence() { _mm_sfence(); }
};