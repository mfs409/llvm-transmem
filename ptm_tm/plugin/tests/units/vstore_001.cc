// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile bool results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_bool(bool);
void tm_test_vstore_bool(bool);
}

extern volatile bool var_bool;

int main() {
    mem_test<bool, bool>("vstore_001", "stores of volatile bools get instrumented",
      {test_vstore_bool, tm_test_vstore_bool}, (bool*)&var_bool, false, true,
      {{TM_STATS_STORE_U1, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile bool var_bool;
extern "C" {
TX_SAFE void test_vstore_bool(bool v) { var_bool = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile bool var_bool = {false};
#endif
