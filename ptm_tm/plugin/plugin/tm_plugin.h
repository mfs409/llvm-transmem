#pragma once

#include <queue>
#include <set>
#include <unordered_map>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "signatures.h"
#include "types.h"

/// tm_plugin is a ModulePass that provides support for transactional memory.
/// It supports three APIs:
///   -- A lambda-based API, in which transactional instrumentation is at the
///      granularity of function bodies.
///   -- A legacy C API, also at the granularity of functions, that passes
///      function pointers and opaque argument pointers to a library.
///   -- An RAII-based API, in which the lifetime of a special "TX" object
///      dictates the lexical scope that requires instrumentation.
///
/// For certain PTM algorithms, it is useful to have a version of the plugin
/// that only instruments writes, not reads.  To support that, we have
class tm_plugin : public llvm::ModulePass {
  /// Every pass must have an ID field
  static char ID;

  /// Configuration option: should this plugin instrument reads and writes, or
  /// just writes?
  const bool instrument_reads = INST_READ;

  /// All of the signatures for types and functions used by the plugin
  signatures sigs;

  /// a collection of all the regions within the module that use the RAII API
  std::vector<raii_region_t> raii_regions;

  /// a worklist for managing the functions that require instrumentation.  These
  /// could be reachable from the RAII API, or could be discovered through the
  /// lambda or legacy API
  std::queue<llvm::Function *> func_worklist;

  /// The set of functions that need instrumentation. This set includes all
  /// of the functions that were explicitly annotated by the programmer, and all
  /// functions reachable from region launch points and annotated functions
  std::unordered_map<llvm::Function *, function_features> functions;

  /// a list of all our suspected lambdas, since they require extra attention
  ///
  /// NB: the use of a vector introduces O(N) lookup overhead later in the code
  std::vector<llvm::Function *> lambdas;

  /// a list of functions that have the tm_pure attribute
  std::vector<llvm::Function *> purelist;

  /// a list of functions that use the tm_rename attribute
  std::unordered_map<llvm::Function *, llvm::Function *> renamelist;

  /// A lookup function for finding things in the functions map
  llvm::Function *get_clone(llvm::Function *input_function) {
    auto function = functions.find(input_function);
    if (function != functions.end())
      return function->second.clone;
    else
      return nullptr;
  }

  /// Discovery: find all annotations in the Module and attach them to the
  /// corresponding functions
  void attach_annotations_to_functions(llvm::Module &M);

  /// Discovery: find the functions in the module that are annotated, and put
  /// them into the worklist
  void discover_annotated_funcs(llvm::Module &M);

  /// Discovery: find the functions that are reached via TM_EXECUTE_C, and put
  /// them into the worklist
  void discover_capi_funcs(llvm::Module &M);

  /// Discovery: Find the lambda-based executions, and put them in the worklist
  /// and lambdas list
  void discover_lambda_funcs(llvm::Module &M);

  /// Discovery: Find all basic blocks that need instrumentation according to
  /// the RAII API, and put them in raii_regions
  void discover_raii(llvm::Module &M);

  /// Discovery: Find the annotated constructors, and put them in the worklist
  void discover_constructor(llvm::Module &M);

  /// Discovery: Process the worklist until it is empty, in order to find all
  /// reachable functions.
  void discover_reachable_funcs();

  /// Discovery: Process the raii_regions list to find
  /// reachable functions.
  void discover_reachable_funcs_raii(llvm::Module &M);

  /// Cloning: clone all functions in the worklist
  void create_clones();

  /// Function Instrumentation: Instrument the instrutions within an RAII region
  void instrument_raii_regions(llvm::Module &M);

  /// Function Instrumentation: helper to transform an instruction in the RAII
  /// api
  void instrument_raii_instruction(llvm::BasicBlock::iterator inst,
                                   llvm::Function::iterator bb);

  /// Function Instrumentation: helper to transform a callsite to use a clone
  llvm::Instruction *transform_callsite(llvm::CallSite &callsite,
                                        llvm::BasicBlock::iterator inst);

  /// Function Instrumentation: helper to replace a call instruction with a new
  /// instruction
  llvm::Instruction *create_callinst(llvm::CallSite &callsite,
                                     llvm::BasicBlock::iterator inst,
                                     llvm::Value *val, llvm::Value *orig_val);

  /// Function Instrumentation: helper to replace an invoke instruction with a
  /// new instruction
  llvm::Instruction *create_invokeinst(llvm::CallSite &callsite,
                                       llvm::BasicBlock::iterator inst,
                                       llvm::Value *val, llvm::Value *orig_val);

  /// Function Instrumentation: helper to insert a call to TM_UNSAFE before the
  /// provided instruction
  void prefix_with_unsafe(llvm::BasicBlock::iterator inst);

  /// Function Instrumentation: helper to try to convert a store instruction
  /// into a tm_store
  llvm::CallInst *convert_store(llvm::StoreInst *store);

  /// Function Instrumentation: helper to try to convert a load instruction into
  /// a tm_load
  llvm::Instruction *convert_load(llvm::LoadInst *load);

  /// Function Instrumentation: helper to try to convert a intrinsic instruction
  /// into a safe call
  void convert_intrinsics(llvm::Function *callee,
                          llvm::BasicBlock::iterator inst);

  /// Function Instrumentation: Main routine for function body transformation
  void instrument_function_bodies();

  /// Boundary Instrumentation: transform region start commands that use the C
  /// API
  void convert_region_begin_c_api(llvm::Module &M);

  /// Boundary Instrumentation: transform the code inside of lambdas
  void convert_lambdas_cxx_api(llvm::Module &M);

  /// Run-time Mappings: add the static initializer that tells the runtime
  /// library about function-to-clone mappings
  void create_runtime_mappings(llvm::Module &M);

  /// Optimization: remove extra unsafe function calls from a basic block
  void optimize_unsafe(llvm::Module &M);

public:
  /// Constructor: call the super's constructor and set up the plugin
  tm_plugin() : ModulePass(ID) {}

  /// Prepare to instrument a module by passing the module to this plugin, so
  /// that we can initialize the plugin with the module.
  ///
  /// NB: We can't do this work in the constructor, because we need a Module
  ///     before the other objects can truly be initialized
  virtual bool doInitialization(llvm::Module &M) override;

  /// Instrument a module
  ///
  /// NB: this assumes it will be passed the same Module as was passed to
  ///     initialize
  virtual bool runOnModule(llvm::Module &M) override;
};