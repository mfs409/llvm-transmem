// Test of std::atomic load instrumentation.
//
// Here we ensure that an atomic load of a float results in a proper call to the
// API

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
float test_aload_f();
float tm_test_aload_f();
}

int main() {
  report<float>("aload_006", "loads of atomic floats get instrumented",
                {{test_aload_f(), tm_test_aload_f()}},
                {{TM_STATS_LOAD_F, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<float> var_f;
extern "C" {
TX_SAFE float test_aload_f() { return var_f == 1.5f ? 1.5f : 0; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<float> var_f = {1.5f};
#endif
