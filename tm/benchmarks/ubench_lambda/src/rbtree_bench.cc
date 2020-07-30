// benchmark harness stuff
#include "../common/config.h"
#include "../common/tests.h"

// red-black tree implementations
#include "rbtree_tm.h"

/// main routine: parse the command-line arguments and then launch the
/// appropriate instantiation of the benchmark template
int main(int argc, char **argv) {
  config *cfg =
      new config("rbtree_bench", "red-black tree intset tests", {"tm"}, "");
  cfg->init_from_args("rbtree", argc, argv);
  cfg->report();

  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<rbtree_tm<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;
  return 0;
}
