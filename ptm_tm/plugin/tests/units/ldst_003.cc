// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a short results in a proper call
// to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_loadstore_u2(short);
void tm_test_loadstore_u2(short);
}

extern short var_u2;

int main() {
    mem_test<short, short>("ldst_003",
                  "loads followed by stores of shorts get instrumented",
                  {test_loadstore_u2, tm_test_loadstore_u2}, &var_u2, 9, 0xCA,
                  {{TM_STATS_STORE_U2, 1}, {TM_STATS_LOAD_U2, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern short var_u2;
extern "C" {
TX_SAFE void test_loadstore_u2(short v) { var_u2 += v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
short var_u2 = 9;
#endif
