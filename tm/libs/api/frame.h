/// frame.h implements the portion of the API required for the stack frame
/// optimization.  It also provides support for commit handlers.

#pragma once

#include "../../common/tm_defines.h"

/// Create the API function that can be explicitly called from a program in
/// order to change the bottom of the transactional stack, and create a function
/// for registering commit handlers.
#define API_TM_STACKFRAME_OPT                                                  \
  extern "C" {                                                                 \
  void TM_SET_STACKFRAME(void *addr) { get_self()->adjustStackBottom(addr); }  \
  void TM_COMMIT_HANDLER(void (*func)(void *), void *args) {                   \
    get_self()->registerCommitHandler(func, args);                             \
  }                                                                            \
  }

/// Create a dummy version of the API function that can be explicitly called
/// from a program in order to change the bottom of the transactional stack.
/// Also provide a correct function for registering commit handlers.
#define API_TM_STACKFRAME_NAIVE                                                \
  extern "C" {                                                                 \
  void TM_SET_STACKFRAME(void *) {}                                            \
  void TM_COMMIT_HANDLER(void (*func)(void *), void *args) {                   \
    get_self()->registerCommitHandler(func, args);                             \
  }                                                                            \
  }