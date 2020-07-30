#pragma once

#include <atomic>
#include <chrono>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "defs.h"

/// experiment_manager keeps track of all data that we measure during an
/// experiment, and any data we use to manage the execution of the experiment
struct experiment_manager {
  /// Barriers for controlling the execution of the program
  std::atomic<unsigned int> barriers[3];

  /// Start time of the experiment
  std::chrono::high_resolution_clock::time_point start_time;

  /// End time of the experiment
  std::chrono::high_resolution_clock::time_point end_time;

  /// These are the actual global statistics
  std::atomic<uint64_t> stats[event_types::COUNT];

  /// The flag used to stop the experiment
  std::atomic<bool> running;

  /// Static reference to singleton instance of this struct
  static experiment_manager *instance;

  /// Construct the global context by initializing the barriers and zeroing the
  /// counters
  experiment_manager() {
    running.store(true);
    experiment_manager::instance = this;
    for (int i = 0; i < 3; ++i)
      barriers[i] = 0;
    for (int i = 0; i < event_types::COUNT; ++i)
      stats[i] = 0;
  }

  /// Report all of the statistics that we counted
  void report(config *cfg) {
    if (cfg->output_raw) {
      report_raw(cfg);
    } else {
      report_pretty(cfg);
    }
  }

  /// Report all of the configuration settings and statistics that we counted as
  /// a comma separated line
  void report_raw(config *cfg) {
    using std::cout;
    using std::endl;
    using namespace std::chrono;

    cfg->report_raw();

    // Print a short marker indicating what the next two fields are that won't
    // interfere with the ability to parse this output as a CSV
    cout << ",(thruput time ops),";

    // Report throughput, execution time, and operations completed
    uint64_t ops = count_operations();
    auto dur = duration_cast<duration<double>>(end_time - start_time).count();
    cout << ops / dur << "," << dur << "," << ops << endl;
  }

  /// Report all of the statistics that we counted for human readability
  void report_pretty(config *cfg) {
    using std::cout;
    using std::endl;
    using namespace std::chrono;

    // Report throughput, execution time, and operations completed
    uint64_t ops = count_operations();
    auto dur = duration_cast<duration<double>>(end_time - start_time).count();
    cout << "Throughput: " << ops / dur << endl;
    cout << "Execution Time: " << dur << endl;
    cout << "Operations: " << ops << endl;

    if (!cfg->verbose)
      return;
    std::string titles[] = {"lookup hit",  "lookup miss", "insert hit",
                            "insert miss", "remove hit",  "remove miss"};
    for (size_t i = 0; i < COUNT; ++i) {
      cout << "  " << titles[i] << " : " << stats[i] << endl;
    }
  }

  /// Before launching experiments, use this to ensure that the threads start at
  /// the same time.  This uses two barriers internally, with a timer read
  /// between the first and second, so that we don't read the time while threads
  /// are still being configured, but we do ensure we read it before any work is
  /// done
  void sync_before_launch(size_t id, config *cfg) {
    // Barrier #1: ensure everyone is initialized
    barrier(0, id, cfg);
    // Now get the time
    if (id == 0) {
      start_time = std::chrono::high_resolution_clock::now();
      if (cfg->timed_mode) {
        signal(SIGALRM, experiment_manager::stop_running);
        alarm(cfg->interval);
      }
    }
    // Barrier #2: ensure we have the start time before work begins
    barrier(1, id, cfg);
  }

  /// Method used to stop test execution.
  static void stop_running(int signal) {
    experiment_manager::instance->running.store(false);
  }

  /// After threads finish the experiments, use this to have them all wait
  /// before getting the stop time.
  void sync_after_launch(size_t id, config *cfg) {
    // wait for all threads
    barrier(2, id, cfg);
    // now get the time
    if (id == 0)
      end_time = std::chrono::high_resolution_clock::now();
  }

  /// Arrive at one of the barriers.
  void barrier(size_t which, size_t id, config *cfg) {
    barriers[which]++;
    while (barriers[which] < cfg->nthreads) {
    }
  }

  /// Get a count of the number of operations that were completed.
  /// Note: this is brittle, be sure to update this when introducing a new
  /// operation type.
  uint64_t count_operations() {
    return stats[LOOKUP_HIT] + stats[LOOKUP_MISS] + stats[INSERT_HIT] +
           stats[INSERT_MISS] + stats[REMOVE_HIT] + stats[REMOVE_MISS];
  }
};

// Provide a definition to go along with the declaration of the singleton
// instance.
//
// NB: this would break if we included this file in more than one .cc file
experiment_manager *experiment_manager::instance;