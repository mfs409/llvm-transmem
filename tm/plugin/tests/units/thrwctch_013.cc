// Test of throw/catch behaviors
//
// This test is not possible yet.  We want to make sure that we don't leak the
// exception object if we abort between the throw and the catch.
//
// The reason the test is not possible is that we don't have a way to cause an
// abort between the throw and the catch.  If we can ever figure out a way to
// cause such an event, we should complete and then activate this test.

#ifdef TEST_DRIVER
#include "../include/harness.h"

int main() {
  report<int>("thrwctch_013",
              "Abort between throw and catch (expect failure... test not "
              "implemented yet)",
              {{-1, 0}}, {{TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#endif

#ifdef TEST_OFILE2
#include "../../../common/tm_api.h"
#endif
