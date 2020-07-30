// Test of malloc/free
//
// Make sure that annotating the constructor for a class works when the
// constructor is in another TU

struct my_t {
  int id;
  my_t(int _id, bool *success);
};

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int test_ctor_new(int, int);
int tm_test_ctor_new(int, int);
}

int main() {
  report<bool>("alloc_005", "Marking an other-TU constructor works correctly",
               {{test_ctor_new(0, 8), tm_test_ctor_new(1, 8)}},
               {{TM_STATS_UNSAFE, 0},
                {TM_STATS_MALLOCS, 1},
                {TM_STATS_TRANSLATE_FOUND, 1}});
}
#endif

#ifdef TEST_OFILE1
// TODO: this test is currently breaking.  I think the issue is that we have not
//       yet called anything from alloc_005.1.o, and therefore the clone table
//       initializer for this .o hasn't been called.  Fixing this issue could be
//       a major challenge.
#include "../../../common/tm_api.h"
TX_SAFE my_t::my_t(int _id, bool *success) {
  id = _id;
  TX_CTOR();
}
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
my_t *myPtr[2];
extern "C" {
TX_SAFE int test_ctor_new(int i, int x) {
  myPtr[i] = new my_t(x, nullptr);
  return myPtr[i]->id;
}
}
// NB: operator new() is not in this TU, so we need to interpose.  For now, we
// will just forward to a call to malloc().
TX_RENAME(_Znwm) void *my_new(size_t size) {
  void *ptr = malloc(size);
  return ptr;
}
TX_RENAME(_ZdlPv) void my_delete(void *ptr) { free(ptr); }
#endif
