// Test of malloc/free
//
// Here we are making sure that operator delete() gets instrumented

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
bool test_delete(int);
bool tm_test_delete(int);
}

int main() {
  report<bool>("alloc_004", "calls to delete lead to instrumented free() calls",
               {{test_delete(0), tm_test_delete(1)}},
               {{TM_STATS_UNSAFE, 0}, {TM_STATS_FREES, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
my_type *my_object[2] = {new my_type(1), new my_type(2)};

TX_RENAME(_ZdlPv) void my_delete(void *ptr) { free(ptr); }
extern "C" {
TX_SAFE void test_delete(int idx) { delete my_object[idx]; }
}
#endif

#ifdef TEST_OFILE2
#endif
