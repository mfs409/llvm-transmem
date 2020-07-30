// Test of cloning facility.
//
// Here we ensure that an annotation on a function declaration results in the
// creation of a clone of the function's definition.  Note that this is an
// early-stage test, so we don't want to check if the clone body is correct.  To
// handle this, the body just returns an int.
//
// NB: Here, and throughout the tests, we will frequently use "C" linkage so
// that it is easy to infer the name that the plugin will give the clone.

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int test_clone_noparam();
int tm_test_clone_noparam();
}

int main() {
  // NB: the real test here is "did the code link" :)
  report<int>("clone_001",
              "Annotating the declaration of a function with no parameters "
              "leads to a clone being made",
              {{test_clone_noparam(), tm_test_clone_noparam()}}, {});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE int test_clone_noparam();
int test_clone_noparam() { return 5; }
}
#endif

#ifdef TEST_OFILE2
// not needed for this test
#endif