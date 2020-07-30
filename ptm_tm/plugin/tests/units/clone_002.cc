// Test of cloning facility.
//
// Here we test that annotating a function *definition* results in the creation
// of a clone.  As in clone_001, this is an early-stage test, so we don't want
// to check if the clone body is correct yet.

#ifdef TEST_DRIVER
#include "../include/harness.h"

// declare the function in OFILE1 that comprises the root of the test
extern "C" {
int test_clone_justdef();
int tm_test_clone_justdef();
}

int main() {
  // NB: As in clone_001, the real test is "did the code link" :)
  report<int>("clone_002",
              "Annotating the definition of a function with no parameters "
              "leads to a clone being made",
              {{test_clone_justdef(), tm_test_clone_justdef()}}, {});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE int test_clone_justdef() { return 50; }
}
#endif

#ifdef TEST_OFILE2
// not needed for this test
#endif