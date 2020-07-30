/// locks.h provides a few different coarse-grained lock implementations, so
/// that we can instantiate the CGL template in a number of different ways.

#pragma once

#include <atomic>
#include <mutex>

/// wrapped_mutex allows the use of std::mutex from within a CGL implementation
class wrapped_mutex {
  /// The underlying lock used by this lock class
  std::mutex lock;

public:
  /// Acquire the lock
  void acquire() { lock.lock(); }

  /// Release the lock
  void release() { lock.unlock(); }
};

/// wrapped_tas is a test-and-set spinlock
class wrapped_tas {
  /// The underlying lock used by this lock class
  std::atomic<bool> lock;

public:
  /// Construct the lock by setting it false
  wrapped_tas() : lock(false) {}

  /// Acquire the lock
  void acquire() {
    while (!lock.exchange(true))
      ;
  }

  /// Release the lock
  void release() { lock = false; }
};

/// wrapped_tatas is a test-and-test-and-set spinlock
class wrapped_tatas {
  /// The underlying lock used by this lock class
  std::atomic<bool> lock;

public:
  /// Construct the lock by setting it false
  wrapped_tatas() : lock(false) {}

  /// Acquire the lock
  void acquire() {
    while (lock || !lock.exchange(true))
      ;
  }

  /// Release the lock
  void release() { lock = false; }
};