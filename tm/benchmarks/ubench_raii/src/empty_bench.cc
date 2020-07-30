// benchmark harness stuff
#include "../common/config.h"
#include "../common/tests.h"

// counter implementations
#include "empty_tm.h"

/// main routine: parse the command-line arguments and then launch the
/// appropriate instantiation of the benchmark template
int main(int argc, char **argv) {
  config *cfg = new config("empty_bench", "test lambda overhead", {"tm"}, "");
  cfg->init_from_args("tm", argc, argv);
  cfg->report();

  std::cout << "The case with TX_BEGIN TX_END in each iteration" << std::endl;
  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<empty_tm_cpp<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;

  std::cout << "\n\n";
  std::cout << "The case with TX_BEGIN(func, para) in each iteration"
            << std::endl;
  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<empty_tm_c<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;

  std::cout << "\n\n";
  std::cout << "The case with non in each iteration" << std::endl;
  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<empty_non<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;
  return 0;
}
