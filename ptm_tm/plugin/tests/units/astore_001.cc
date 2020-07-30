// Test of std::atomic store instrumentation.
//
// Here we ensure that a store of an atomic bool results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_astore_bool(bool);
void tm_test_astore_bool(bool);
}

extern std::atomic<bool> var_bool;

int main() {
    mem_test<bool, bool>("astore_001", "stores of atomic bools get instrumented",
      {test_astore_bool, tm_test_astore_bool}, (bool*)&var_bool, false, true,
      {{TM_STATS_STORE_U1, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<bool> var_bool;
extern "C" {
TX_SAFE void test_astore_bool(bool v) { var_bool = v; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<bool> var_bool = {false};
#endif
