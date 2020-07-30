// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a bool results in the proper
// calls to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_loadstore_bool(bool);
void tm_test_loadstore_bool(bool);
}

extern bool var_bool;

int main() {
    mem_test<bool, bool>("ldst_001",
                 "loads followed by stores of bools get instrumented",
                 {test_loadstore_bool, tm_test_loadstore_bool}, &var_bool,
                 false, true, {{TM_STATS_STORE_U1, 1}, {TM_STATS_LOAD_U1, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern bool var_bool;
extern "C" {
TX_SAFE void test_loadstore_bool(bool v) { var_bool |= v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
bool var_bool = true;
#endif
