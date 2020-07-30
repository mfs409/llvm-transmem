// Test of atomic operations.  For TM, atomic operations are UNSAFE.
//
// Atomic FAI should produce unsafe

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <atomic>
std::atomic<int> ato_u4{0};
extern "C" {
int afai_callable(int i);
int tm_afai_callable(int i);
}
int main() {
  report<int>("atomic_003", "atomic fai should be unsafe",
              {{afai_callable(0), tm_afai_callable(0)}},
              {{TM_STATS_LOAD_U4, 0}, {TM_STATS_UNSAFE, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <atomic>
extern std::atomic<int> ato_u4;
extern "C" {
TX_SAFE int afai_callable(int i) { return ato_u4.fetch_add(i); }
}
#endif

#ifdef TEST_OFILE2
#endif