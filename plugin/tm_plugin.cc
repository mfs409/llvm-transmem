#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "../include/tm_api.h"

#include "tm_plugin.h"

using namespace llvm;
using namespace llvm::legacy;

/// Prepare to instrument a module by initializing our signatures object and
/// attaching annotations to functions.
///
/// NB: we assume that this modifies the module, so we always return true
bool tm_plugin::doInitialization(Module &M) {
  sigs.init(M);
  attach_annotations_to_functions(M);
  return true;
}

/// Instrument a module by executing all of the tm_plugin tasks on it.  See the
/// documentation in tm_plugin.h for more details on the steps of the algorithm
///
/// NB: this assumes that M is the same module as was passed to initialize()
///
/// NB: we assume that this modifies the module, so we always return true
bool tm_plugin::runOnModule(Module &M) {
  // Discovery
  discover_annotated_funcs(M);
  discover_capi_funcs(M);
  discover_lambda_funcs(M);
  discover_constructor(M);
  discover_reachable_funcs();

  // Cloning and Function Body Instrumentation
  create_clones();
  instrument_function_bodies();

  // Boundary Instrumentation
  convert_region_begin_c_api(M);
  convert_lambdas_cxx_api(M);

  // Optimizations
  optimize_unsafe(M);

  // Add a static initializer to the module, so that the run-time system
  // can see the function-to-clone mapping
  create_runtime_mappings(M);

  return true;
}

// Auto-register this pass for -O0 and -O3 builds (see
// http://www.cs.cornell.edu/~asampson/blog/llvm.html)
char tm_plugin::ID = 0;
static void register_plugin(const PassManagerBuilder &, PassManagerBase &PM) {
  PM.add(new tm_plugin());
}
static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                   register_plugin);
static RegisterStandardPasses
    RegisterMyPass0(PassManagerBuilder::EP_EnabledOnOptLevel0, register_plugin);