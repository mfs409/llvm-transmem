// Test of LLVM IR for vector loads and stores
//
// When we do not explicitly use sse data types, we do not end up with the sort
// of vector instructions that lead to not-yet-supported 128-bit+ loads and
// stores.  Note that we still take unsafes on the sqrt calls

#ifdef TEST_DRIVER
#include "../include/harness.h"
extern "C" {
float do_sqrt(float *, int);
float tm_do_sqrt(float *, int);
}
int main() {
  int N = 4;
  float *a;
  posix_memalign((void **)&a, 16, N * sizeof(float));
  for (int i = 0; i < N; ++i)
    a[i] = i;
  float r1 = do_sqrt(a, N);
  for (int i = 0; i < N; ++i)
    a[i] = i;
  float r2 = tm_do_sqrt(a, N);
  report<int>(
      "ssevec_001", "Vectors of floats do not produce unsafe SSE", {{r1, r2}},
      {{TM_STATS_LOAD_F, 8}, {TM_STATS_STORE_F, 4}, {TM_STATS_UNSAFE, N}});
}
#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
#include <cmath>
extern "C" {
TX_SAFE float do_sqrt(float *a, int N) {
  float sum = 0;
  for (int i = 0; i < N; ++i)
    a[i] = sqrt(a[i]);
  for (int i = 0; i < N; ++i)
    sum = sum + a[i];
  return sum;
}
}
#endif

#ifdef TEST_OFILE2
#endif