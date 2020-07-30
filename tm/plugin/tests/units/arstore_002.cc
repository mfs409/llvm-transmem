// Test of std::atomic relaxed  store instrumentation.
//
// Here we ensure that a relaxed store of an atomic char results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_arstore_u1(char);
void tm_test_arstore_u1(char);
}

extern std::atomic<char> var_u1;

int main() {
    mem_test<char, char>("arstore_002", "relaxed stores of atomic chars get instrumented",
      {test_arstore_u1, tm_test_arstore_u1}, (char*)&var_u1, 0, 0xCA,
      {{TM_STATS_STORE_U1, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<char> var_u1;
extern "C" {
    TX_SAFE void test_arstore_u1(char v) { var_u1.store(v, std::memory_order_relaxed); }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<char> var_u1 = {0};
#endif
