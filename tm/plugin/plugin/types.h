#pragma once

#include <vector>

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

/// raii_region_t tracks a lexically-scoped region that uses the RAII API.  It
/// consists of the instruction of the constructor, the instruction of the
/// destructor, and a collection of all basic blocks between them.
struct raii_region_t {
  /// Constructor instruction for a RAII region
  llvm::Instruction *ctor_inst = nullptr;

  /// destructor instruction for a RAII region
  llvm::Instruction *dtor_inst = nullptr;

  /// all BBs between the constructor and destructor
  std::vector<llvm::BasicBlock *> instruction_blocks;

  /// Default constructor makes an instance with no fields set
  raii_region_t() {}

  /// Construct an instance with a ctor and dtor pre-set
  raii_region_t(llvm::Instruction *ctor, llvm::Instruction *dtor)
      : ctor_inst(ctor), dtor_inst(dtor) {}
};