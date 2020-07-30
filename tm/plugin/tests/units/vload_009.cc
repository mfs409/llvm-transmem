// Test of volatile load instrumentation.
//
// Here we ensure that a load of a volatile pointer to struct results in a proper
// call to the API

// A struct, so we can have pointers to it
struct simple_struct_t {
  int a;
  int b;
  simple_struct_t *next;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
simple_struct_t *test_vload_p();
simple_struct_t *tm_test_vload_p();
}

int main() {
  // NOT STARTED YET
  report<simple_struct_t *>("vload_009",
                            "loads of volatile pointers get instrumented",
                            {{test_vload_p(), tm_test_vload_p()}},
                            {{TM_STATS_LOAD_P, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern simple_struct_t * volatile var_p;
extern "C" {
TX_SAFE simple_struct_t *test_vload_p() {
  return var_p == (simple_struct_t *)0x778899aabbccddee
             ? (simple_struct_t *)0x778899aabbccddee
             : 0;
}
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
simple_struct_t * volatile var_p = (simple_struct_t *)0x778899aabbccddee;
#endif
