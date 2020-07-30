// Test of LLVM IR for masked vector operations
//
// LLVM turns masked vector IR into loads and stores early, so we can easily
// handle vector loads

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int masked_load_test(int *, int *, int *, int);
int tm_masked_load_test(int *, int *, int *, int);
}

int main() {
  int trigger[] = {1, -2, 6, 10};
  int index[] = {0, 1, 2, 3};
  int size = 4;
  int A[] = {0};
  report<int>("maskvec_001",
              "Masked load gets lowered to loads and stores, is safe",
              {{masked_load_test(A, trigger, index, size),
                tm_masked_load_test(A, trigger, index, size)}},
              {{TM_STATS_LOAD_U4, 8}, {TM_STATS_STORE_U4, 4}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE int masked_load_test(int *a, int *b, int *c, int size) {
  for (int i = 0; i < size; i++)
    a[i] = b[c[i]];
  return 1;
}
}
#endif

#ifdef TEST_OFILE2
#endif
