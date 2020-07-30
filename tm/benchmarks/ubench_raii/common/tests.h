/// tests.h defines any templated functions that we use as microbenchmarks on
/// data structures.  Currently there are two tests, for data structures that
/// present a set interface or a map interface

#pragma once

#include <signal.h>
#include <thread>
#include <unistd.h>

#include "config.h"
#include "defs.h"
#include "experiment_manager.h"
#include "thread_context.h"

/// Run tests on data structures that implement a set interface.  This requires
/// set_t to have insert, lookup, and remove operations, and to operate on keys
/// (not key/value pairs)
template <class set_t> void intset_test(config *cfg) {
  using std::cout;
  using std::endl;
  using std::thread;
  using std::vector;
  using namespace std::chrono;

  // Create a global stats object for managing this experiment
  experiment_manager exp;

  // Create a set and initialize it to 50% full
  // NB: insert in reverse order, for the sake of linked lists
  set_t SET(cfg);
  for (size_t i = cfg->key_range + 1; i > 1; i -= 2) {
    // TODO: I don't think this is doing what we want it to do.
    SET.insert(i - 3);
  }

  // This is the benchmark task
  auto task = [&](int id) {
    // The thread's PRNG and counters are here
    thread_context self(id);

    // set up distributions for our PRNG
    using std::uniform_int_distribution;
    uniform_int_distribution<size_t> key_dist(0, cfg->key_range - 1);
    uniform_int_distribution<size_t> action_dist(0, 100);

    // set up the code that is common to timed and fixed-run tests
    auto code = [&]() {
      // Randomly select among lookup/insert/remove
      size_t action = action_dist(self.mt);
      size_t key = key_dist(self.mt) % cfg->key_range;
      // Lookup?
      if (action <= cfg->lookup) {
        if (SET.contains(key)) {
          self.stats[LOOKUP_HIT]++;
        } else {
          self.stats[LOOKUP_MISS]++;
        }
      }
      // Insert?
      else if (action < cfg->lookup + (100 - cfg->lookup) / 2) {
        if (SET.insert(key)) {
          self.stats[INSERT_HIT]++;
        } else {
          self.stats[INSERT_MISS]++;
        }
      }
      // Remove?
      else {
        if (SET.remove(key)) {
          self.stats[REMOVE_HIT]++;
        } else {
          self.stats[REMOVE_MISS]++;
        }
      }
    };

    // Synchronize threads and get time
    exp.sync_before_launch(id, cfg);

    if (cfg->timed_mode) {
      // Run a fixed number of randomly-chosen operations
      while (exp.running.load()) {
        code();
      }
    } else {
      for (size_t i = 0; i < cfg->interval; ++i) {
        code();
      }
    }
    // arrive at the last barrier, then get the timer again
    exp.sync_after_launch(id, cfg);

    // merge stats into global
    for (size_t i = 0; i < event_types::COUNT; ++i) {
      exp.stats[i].fetch_add(self.stats[i]);
    }
  };

  // Launch the threads... this thread won't run the tests
  vector<thread> threads;
  for (size_t i = 0; i < cfg->nthreads; i++)
    threads.emplace_back(task, i);
  for (size_t i = 0; i < cfg->nthreads; i++)
    threads[i].join();

  // always check correctness before reporting stats
  if (cfg->verbose)
    SET.check();

  // Report statistics from the experiment
  exp.report(cfg);
}
