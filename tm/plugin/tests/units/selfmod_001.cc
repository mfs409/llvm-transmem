// Test of dynamic code generation / self-modifying code
//
// Here we show that self-modifying code in C/C++ isn't done through LLVM IR, so
// we don't need special tests.
#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
void setup();
extern "C" {
int test_selfmod();
int tm_test_selfmod();
}

int main() {
  setup();
  report<int>("selfmod_001", "C++ self-generating code is unsafe",
              {{test_selfmod(), tm_test_selfmod()}}, {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
char *data;
extern "C" {
TX_SAFE int test_selfmod() {
  int (*mine)(int) = (int (*)(int))data;
  int x = mine(7);
  return x;
}
}
void setup() {
  // We need 4 bytes for our "function", but we can't mprotect the region to
  // make it executable unless we have a page-aligned address
  data = (char *)malloc(4096 + 0x18);
  uintptr_t addr = (uintptr_t)data + 4096;
  addr = addr & ~4095;
  data = (char *)addr;
  mprotect(data, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  data[0x00] = 0x55;
  data[0x01] = 0x48;
  data[0x02] = 0x89;
  data[0x03] = 0xe5;
  data[0x04] = 0x89;
  data[0x05] = 0x7d;
  data[0x06] = 0xec;
  data[0x07] = 0x90;
  data[0x08] = 0x8b;
  data[0x09] = 0x45;
  data[0x0A] = 0xec;
  data[0x0B] = 0x83;
  data[0x0C] = 0xc0;
  data[0x0D] = 0x01;
  data[0x0E] = 0x89;
  data[0x0F] = 0x45;
  data[0x10] = 0xfc;
  data[0x11] = 0x90;
  data[0x12] = 0x90;
  data[0x13] = 0x8b;
  data[0x14] = 0x45;
  data[0x15] = 0xfc;
  data[0x16] = 0x5d;
  data[0x17] = 0xc3;
}
#endif

#ifdef TEST_OFILE2
/// test() is a reference function.  When we disassemble it, we get the
/// following code:
///
///  0:   55                      push   %rbp
///  1:   48 89 e5                mov    %rsp,%rbp
///  4:   89 7d ec                mov    %edi,-0x14(%rbp)
///  7:   90                      nop
///  8:   8b 45 ec                mov    -0x14(%rbp),%eax
///  b:   83 c0 01                add    $0x1,%eax
///  e:   89 45 fc                mov    %eax,-0x4(%rbp)
/// 11:   90                      nop
/// 12:   90                      nop
/// 13:   8b 45 fc                mov    -0x4(%rbp),%eax
/// 16:   5d                      pop    %rbp
/// 17:   c3                      retq
int test(int x) {
  asm volatile("nop");
  int ret = x + 1;
  asm volatile("nop");
  asm volatile("nop");
  return ret;
}
#endif