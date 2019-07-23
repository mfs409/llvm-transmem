/// stats.h implements the portion of the API required for reporting statistics
/// gathered by the TM algorithm

#pragma once

/// Create the API functions that can be explicitly called from a program in
/// order to report stats.  This version is for TM implementations that don't
/// actually count stats.
#define API_TM_STATS_NOP                                                       \
  extern "C" {                                                                 \
  void TM_REPORT_ALL_STATS() {}                                                \
  }