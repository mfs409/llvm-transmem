// Test of load instrumentation.
//
// Here we ensure that a load of an int results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int test_load_u4();
int tm_test_load_u4();
}

int main() {
  report<int>("load_004", "loads of ints get instrumented",
              {{test_load_u4(), tm_test_load_u4()}}, {{TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern int var_u4;
extern "C" {
TX_SAFE int test_load_u4() { return var_u4 == 0xD7D7D7D7 ? 0xD7D7D7D7 : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
int var_u4 = 0xD7D7D7D7;
#endif
