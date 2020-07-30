#pragma once

#include <cstdint>

#include "minivector.h"
#include "vlog_t.h"

/// ValueLog is really just a vector of address/value pairs that are used by TMs
/// in the NOrec family (value-based validation) to determine if any of the
/// reads made by the TM have become invalid.
class ValueLog_Nonatomic {
  /// The vector that stores values to validate
  MiniVector<vlog_t> valuelog;

public:
  /// Construct a ValueLog with a default capacity
  ValueLog_Nonatomic(const unsigned long _capacity = 64)
      : valuelog(_capacity) {}

  /// Clear the ValueLog
  void clear() { valuelog.clear(); }

  /// Return whether the ValueLog is empty or not
  bool empty() { return valuelog.empty(); }

  /// Insert a vlog_t into the valuelog
  void push_back(vlog_t data) { valuelog.push_back(data); }

  /// Report the array size (e.g., for empty tests)
  unsigned long size() const { return valuelog.size(); }

  /// Validate the valuelog by making sure that each value is the same as when
  /// we read it
  ///
  /// NB: for the "nonatomic" valuelog, this is prone to races in the C++ memory
  ///     model
  bool validate_atomic() {
    bool valid = true;
    for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
      valid = valid && it->checkValue();
    }
    return valid;
  }

  /// Validate the value log, for us in situations where there is no concern
  /// about races with concurrent writes (e.g., while becoming irrevocable).
  bool validate_nonatomic() {
    bool valid = true;
    for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
      valid = valid && it->checkValue();
    }
    return valid;
  }

  /// In some algorithms (e.g., Cohorts), we want validation to exit as quickly
  /// as possible on a failure.  These algorithms also don't require atomic
  /// accesses during validation.
  bool validate_fastexit_nonatomic() {
    for (auto it = valuelog.begin(), e = valuelog.end(); it != e; ++it) {
      if (!it->checkValue())
        return false;
    }
    return true;
  }
};
