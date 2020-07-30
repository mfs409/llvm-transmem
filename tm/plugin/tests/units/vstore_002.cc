// Test of volatile store instrumentation.
//
// Here we ensure that a store of a volatile char results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_vstore_u1(char);
void tm_test_vstore_u1(char);
}

extern volatile char var_u1;

int main() {
    mem_test<char, char>("vstore_002", "stores of volatile chars get instrumented",
      {test_vstore_u1, tm_test_vstore_u1}, (char*)&var_u1, 0, 0xCA,
      {{TM_STATS_STORE_U1, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern volatile char var_u1;
extern "C" {
TX_SAFE void test_vstore_u1(char v) { var_u1 = v; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
volatile char var_u1 = {0};
#endif
