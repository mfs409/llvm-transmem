#pragma once

#include <xmmintrin.h>

/// ADR provides fence and flush instructions that are appropriate when a PTM
/// algorithm is running on an Intel Optane system (real or simulated) that only
/// has enough reserve power to flush the memory controller's write queues.
/// Consequently, a program must explicitly move cache lines to the controller
/// (via clwb), and must order those clwb instructions with regard to subsequent
/// stores (via sfence).
class ADR {
public:
  /// clwb is the instruction for flushing a cache line to the memory
  /// controller.  In ADR, it corresponds 1:1 with the clwb assembly instruction
  static void clwb(void *addr) {
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
  }

  /// sfence is the instruction for ensuring store-store ordering.  In eADR, it
  /// is a no-op
  static void sfence() { _mm_sfence(); }
};

/// eADR provides no-op fence and flush instructions, which are appropriate when
/// a PTM algorithm is running on an Intel Optane system (real or simulated)
/// that has enough reserve power to flush any visible pmem stores to the NVM.
/// Visible stores are those in the L2 and L3 that map to physical pages of
/// Optane memory.  Since there is a lot of reserve power, the flush and fence
/// instructions can be no-ops.
class eADR {
public:
  /// clwb is the instruction for flushing a cache line to the memory
  /// controller.  In ADR, it corresponds 1:1 with the clwb assembly instruction
  static void clwb(void *addr) {
    asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
  }

  /// sfence is the instruction for ensuring store-store ordering.  In eADR, it
  /// is a no-op
  static void sfence() { _mm_sfence(); }
};