// Test of load and store instrumentation.
//
// Here we ensure that a load and then store of a pointer to struct results in a
// proper call to the API

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
void test_loadstore_p(simple_struct_t *);
void tm_test_loadstore_p(simple_struct_t *);
}

extern simple_struct_t *var_p;
extern simple_struct_t mine;

int main() {
    mem_test<simple_struct_t *, simple_struct_t *>(
      "ldst_009", "loads followed by stores of pointers get instrumented",
      {test_loadstore_p, tm_test_loadstore_p}, &var_p, &mine, &mine,
      {{TM_STATS_STORE_P, 1}, {TM_STATS_LOAD_P, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern simple_struct_t *var_p;
extern "C" {
TX_SAFE void test_loadstore_p(simple_struct_t *v) { var_p = v->next; }
}
#endif

#ifdef TEST_OFILE2
// declare var here, to ensure it doesn't get optimized away
simple_struct_t mine = {6, 7, (simple_struct_t *)0xCAFEBABE};
simple_struct_t *var_p = &mine;
#endif
