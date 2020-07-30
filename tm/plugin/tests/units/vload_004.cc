// Test of volatile load instrumentation.
//
// Here we ensure that a volatile load of an int results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int test_vload_u4();
int tm_test_vload_u4();
}

int main() {
  report<int>("vload_004", "loads of volatile ints get instrumented",
              {{test_vload_u4(), tm_test_vload_u4()}},
              {{TM_STATS_LOAD_U4, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile unsigned int var_u4;
extern "C" {
TX_SAFE int test_vload_u4() { return var_u4 == 0xD7D7D7D7 ? 0xD7D7D7D7 : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile unsigned int var_u4 = {0xD7D7D7D7};
#endif
