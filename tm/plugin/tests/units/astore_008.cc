// Test of std::atomic store instrumentation.
//
// Here we ensure that a store of an atomic pointer to struct results in a
// proper call to the API

// A struct, so we can have pointers to it
struct simple_struct_t {
  int a;
  int b;
  simple_struct_t *next;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
void test_astore_p(simple_struct_t *);
void tm_test_astore_p(simple_struct_t *);
}

extern std::atomic<simple_struct_t *> var_p;

int main() {
  mem_test<simple_struct_t *, simple_struct_t *>(
      "astore_008", "stores of atomic pointers get instrumented",
      {test_astore_p, tm_test_astore_p}, (simple_struct_t **)&var_p, nullptr,
      (simple_struct_t *)0xCAFF00BABAL,
      {{TM_STATS_STORE_P, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<simple_struct_t *> var_p;
extern "C" {
TX_SAFE void test_astore_p(simple_struct_t *v) { var_p = v; }
}
#endif

#ifdef TEST_OFILE2
#include <atomic>
// declare var here, to ensure it doesn't get optimized away
std::atomic<struct store_struct *> var_p = {nullptr};
#endif
