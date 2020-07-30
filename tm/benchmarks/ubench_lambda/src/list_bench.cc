// benchmark harness stuff
#include "../common/config.h"
#include "../common/tests.h"

// list implementations
#include "list_tm.h"

/// main routine: parse the command-line arguments and then launch the
/// appropriate instantiation of the benchmark template
int main(int argc, char **argv) {
  config *cfg =
      new config("list_bench", "linked list intset tests", {"tm"}, "");
  cfg->init_from_args("list", argc, argv);
  cfg->report();

  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<list_tm<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;
  return 0;
}