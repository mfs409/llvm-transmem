// Test of LLVM IR for insertvector and shufflevector
//
// This code produces extractElement, insertElement, and shuffleVector, which
// should all be safe.  But there is also an unsafe vector load.

typedef int v4si __attribute__((__vector_size__(16)));

#ifdef TEST_DRIVER
#include "../include/harness.h"
#include <iostream>

extern "C" {
v4si test_insert_vector(v4si *vec, int i, int j);
v4si tm_test_insert_vector(v4si *vec, int i, int j);
}

int main() {
  v4si v = {3, 5, 7, 8};
  v4si r1 = test_insert_vector(&v, 0, 2);
  v4si r2 = tm_test_insert_vector(&v, 0, 2);
  bool res = true;
  for (int i = 0; i < 4; ++i)
    res &= r1[i] == r2[i];
  report<bool>("ivsv_001", "element and shuffle operations on vectors are safe",
               {{res, true}}, {{TM_STATS_LOAD_U4, 0}, {TM_STATS_UNSAFE, 1}});
}

#endif

#ifdef TEST_OFILE1
#include "../../../common/tm_api.h"
extern "C" {
TX_SAFE v4si test_insert_vector(v4si *vec, int i, int j) {
  int in = vec[i][j];
  return (v4si){in, in, in, in};
}
}
#endif

#ifdef TEST_OFILE2
#endif