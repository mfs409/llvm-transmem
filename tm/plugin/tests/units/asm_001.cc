// Test of inline assembly.
//
// Inline assembly is not safe for transactional code, so we expect any inline
// assembly to be dominated by a call to TM_UNSAFE.  We can't test for
// domination at run-time, so we settle for testing if TM_UNSAFE was called at
// all.

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int asm_test();
int tm_asm_test();
}

// main function just calls the test and reports the result
int main() {
  report<int>("asm_001",
              "Ensure that inline assembly in a clone leads to an unsafe call",
              {{asm_test(), tm_asm_test()}}, {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE int asm_test() {
  int res = 0, op1 = 20, op2 = 30;
  __asm__("addl %%ebx, %%eax;" : "=a"(res) : "a"(op1), "b"(op2));
  return res;
}
}
#endif

#ifdef TEST_OFILE2
// not needed for this test
#endif