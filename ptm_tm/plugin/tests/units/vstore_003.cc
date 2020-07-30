// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile short results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_u2(short);
void tm_test_vstore_u2(short);
}

extern volatile short var_u2;

int main() {
    mem_test<short, short>("vstore_003", "stores of volatile shorts get instrumented",
      {test_vstore_u2, tm_test_vstore_u2}, (short*)&var_u2, 0, 0xCAFE,
      {{TM_STATS_STORE_U2, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile short var_u2;
extern "C" {
TX_SAFE void test_vstore_u2(short v) { var_u2 = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile short var_u2 = {0};
#endif
