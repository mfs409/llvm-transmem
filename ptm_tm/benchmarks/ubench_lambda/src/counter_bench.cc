// benchmark harness stuff
#include "../common/config.h"
#include "../common/tests.h"

// counter implementations
#include "counter_tm.h"

/// main routine: parse the command-line arguments and then launch the
/// appropriate instantiation of the benchmark template
int main(int argc, char **argv) {
  config *cfg = new config("counter_bench", "shared counter tests", {"tm"}, "");
  cfg->key_range = 1; // override default value, but allow args to re-override
  cfg->init_from_args("counter", argc, argv);
  cfg->report();

  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<counter_tm<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;
  return 0;
}