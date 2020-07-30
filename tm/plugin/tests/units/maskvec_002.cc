// Test of LLVM IR for masked vector operations
//
// For now, vector load/store operations get prefixed with UNSAFE.  Note that
// we *are* seeing scatter/gather in the LLVM IR, but by the time the plugin is
// seeing the code, it is already loads of 128-bit fields, which we do not yet
// support.

typedef int v4si __attribute__((__vector_size__(16)));
typedef float v4sf __attribute__((__vector_size__(16)));

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int masked_gather_test(int *, v4sf *, int *, v4si *, int, int);
int tm_masked_gather_test(int *, v4sf *, int *, v4si *, int, int);
}

int main() {
  int trigger[] = {1, -2, 6, 10};
  int index[] = {0, 1, 2, 3};
  v4si in[4] = {12, 54, 87, 38};
  v4sf out[4] = {0.0, 0.0, 0.0, 0.0};
  int size = 4;
  report<int>(
      "maskvec_002", "Masked gather becomes unsafe vector loads",
      {{masked_gather_test(trigger, out, index, in, size, size - 5),
        tm_masked_gather_test(trigger, out, index, in, size, size - 5)}},
      {{TM_STATS_LOAD_U4, 7}, {TM_STATS_STORE_U4, 0}, {TM_STATS_UNSAFE, 3}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE int masked_gather_test(int *trigger, v4sf *out, int *index, v4si *in,
                               int size, int value) {
  for (unsigned i = 0; i < size; i++)
    if (trigger[i] > value)
      out[i] = in[index[i]];
  return 1;
}
}
#endif

#ifdef TEST_OFILE2
#endif
