// Test of malloc/free
//
// Here we are making sure that operator new instruments correctly.  This
// requires us to use a rename to get to the

class my_type {
  int val;
  int val2;

public:
  my_type(int i) : val(i) { val2 = i * i + i; }
  int get_val() { return val; }
};

#ifdef TEST_DRIVER
#include "../include/harness.h"

extern "C" {
int test_new(int);
int tm_test_new(int);
}

int main() {
  report<int>("alloc_003",
              "calls to operator new() lead to instrumented mallocs",
              {{test_new(0), tm_test_new(1)}},
              {{TM_STATS_UNSAFE, 0}, {TM_STATS_MALLOCS, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
my_type *my_object[2];
extern "C" {
TX_SAFE int test_new(int idx) {
  my_object[idx] = new my_type(7);
  return my_object[idx]->get_val();
}
}
// NB: operator new() is not in this TU, so we need to interpose.  For now, we
// will just forward to a call to malloc().
TX_RENAME(_Znwm) void *my_new(size_t size) {
  void *ptr = malloc(size);
  return ptr;
}
#endif

#ifdef TEST_OFILE2
#endif