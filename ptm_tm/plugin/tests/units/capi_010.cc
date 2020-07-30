// Test execution of transactions via the C API
//
// Here we make sure that clones of weak symbols don't cause link errors

// A basic template, but it is noinline, so that we can be sure that it produces
// a weak symbol when we use it
template <class T> __attribute__((noinline)) int get_field(T *obj) {
  return obj->field;
}
// A type that we can use when instantiating the template
struct my_type {
  int field;
};

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
int template_test();

int main() {
  report<int>("capi_010", "cloning template instantiations works",
              {{template_test(), 9}},
              {{TM_STATS_UNSAFE, 0}, {TM_STATS_LOAD_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
int ans;
TX_SAFE void template_target(void *args) {
  my_type *mt = (my_type *)args;
  ans = get_field<my_type>(mt);
}
int template_test() {
  my_type mt = {9};
  TX_BEGIN_C(nullptr, template_target, &mt);
  return ans;
}
#endif

#ifdef TEST_OFILE2
#endif