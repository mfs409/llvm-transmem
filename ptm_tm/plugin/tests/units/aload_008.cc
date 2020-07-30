// Test of std::atomic load instrumentation.
//
// Here we ensure that a load of an atomic pointer to struct results in a proper
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
simple_struct_t *test_aload_p();
simple_struct_t *tm_test_aload_p();
}

int main() {
  // NOT STARTED YET
  report<simple_struct_t *>("aload_008",
                            "loads of atomic pointers get instrumented",
                            {{test_aload_p(), tm_test_aload_p()}},
                            {{TM_STATS_LOAD_P, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<simple_struct_t *> var_p;
extern "C" {
TX_SAFE simple_struct_t *test_aload_p() {
  return var_p == (simple_struct_t *)0x778899aabbccddee
             ? (simple_struct_t *)0x778899aabbccddee
             : 0;
}
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<simple_struct_t *> var_p = {(simple_struct_t *)0x778899aabbccddee};
#endif
