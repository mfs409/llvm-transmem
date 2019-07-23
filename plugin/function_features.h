#pragma once

#include "llvm/IR/Function.h"

/// function_features is a packet of information that we associate with each
/// function that is reachable from an instrumented region.
struct function_features {
  /// The original function
  llvm::Function *orig = nullptr;

  /// The clone of the original function
  llvm::Function *clone = nullptr;

  /// Is orig a lambda?  If so, we must do some work to it, too...
  bool orig_lambda = false;
};