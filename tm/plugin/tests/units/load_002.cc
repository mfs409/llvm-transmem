// Test of load instrumentation.
//
// Here we ensure that a load of a char (u1) results in a proper call to the API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
char test_load_u1();
char tm_test_load_u1();
}

int main() {
  report<char>("load_002", "loads of chars get instrumented",
               {{test_load_u1(), tm_test_load_u1()}}, {{TM_STATS_LOAD_U1, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern char var_u1;
extern "C" {
TX_SAFE char test_load_u1() { return var_u1 == (char)0xF9 ? 0xF9 : 0; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
char var_u1 = 0xF9;
#endif
