// benchmark harness stuff
#include "../common/config.h"
#include "../common/tests.h"

// hashtable implementations
#include "hash_tm.h"

/// main routine: parse the command-line arguments and then launch the
/// appropriate instantiation of the benchmark template
int main(int argc, char **argv) {
  config *cfg = new config("hash_bench", "hashtable intset tests", {"tm"}, "");
  cfg->init_from_args("hash", argc, argv);
  cfg->report();

  // Launch the appropriate test
  if (cfg->bench_name == "tm")
    intset_test<hash_tm<int>>(cfg);
  else
    std::cout << "Invalid benchmark name" << std::endl;
  return 0;
}