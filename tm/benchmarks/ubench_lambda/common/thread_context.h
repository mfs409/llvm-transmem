#pragma once

#include <random>

#include "defs.h"

/// thread_context keeps track of intermediate measurements by a thread, as well
/// as the thread's pseudorandom number generator.
struct thread_context {
  /// A pseudorandom number generator (PRNG)
  std::mt19937 mt;

  /// The thread's id
  int id;

  /// The thread's local statistic counts
  unsigned int stats[event_types::COUNT] = {0};

  /// Construct a thread's context by storing its ID and creating its PRNG
  thread_context(int _id) : mt(_id * LARGE_PRIME), id(_id) {}
};