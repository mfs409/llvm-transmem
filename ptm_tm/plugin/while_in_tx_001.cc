// Test of C++ lambdas for transaction boundaries
//
// Ensure that bodies of lambdas get instrumented

#ifdef TEST_DRIVER
#include "../include/harness.h"
int loadstore_lambda_test();
int main() {
  report<int>("lambda_002", "Lambda body loads/stores get instrumented",
              {{loadstore_lambda_test(), 55}}, {{TM_STATS_LOAD_U4, 1},{TM_STATS_STORE_U4, 1}});
}
#endif

#ifdef TEST_OFILE1
#include "../../include/tm.h"
int x;
int loadstore_lambda_test() {
  x = 55;
  int tmp = 0;
    TX_BEGIN{

        int a  = 10;
        int* y = &a;
        while(y != NULL && *y != 4) {
            TX_BEGIN{
            a --;
            }TX_END;
        }
        x = *y;
        tmp = x;
  //      switch(x) {
  //          case 53: x += 1 ;break;
  //          case 54: x += 2 ;break;
  //          case 55: x += 3 ;break;
  //          default: ;
  //      }
    }TX_END;
  return tmp;
}
#endif

#ifdef TEST_OFILE2
#endif
