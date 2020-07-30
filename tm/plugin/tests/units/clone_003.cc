// Test of cloning facility.
//
// Here we test that cloning doesn't break for some weird reason when we have
// annotations on a function that has parameters.  As in clone_001, this is an
// early-stage test, so we don't want to check if the clone body is correct yet.

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
double test_clone_param(int, float, char, double, long long int, void **);
double tm_test_clone_param(int, float, char, double, long long int, void **);
}

int main() {
  // If this test passes, then the parameters were all sent to the two functions
  // in the same way, and they produced the same value
  void *dummy;
  report<double>(
      "clone_003",
      "Annotating a function with many parameters of varying types "
      "leads to a clone being made",
      {{test_clone_param(5, 4.4, 'a', 7.799998987, 2LL << 40, &dummy),
        tm_test_clone_param(5, 4.4, 'a', 7.799998987, 2LL << 40, &dummy)}},
      {});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE double test_clone_param(int i, float f, char c, double d,
                                long long int lli, void **p) {
  double res = 2 * i + f + c + d + lli;
  res += (uintptr_t)p;
  return res;
}
}
#endif

#ifdef TEST_OFILE2
// not needed for this test
#endif