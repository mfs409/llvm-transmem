#pragma once

#include <queue>
#include <unordered_map>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Module.h"

#include "function_features.h"
#include "signatures.h"

/// tm_plugin is a ModulePass that provides support for transactional memory.
/// It performs the following tasks:
///
/// - Discovery of instrumentable functions.
///   -- Any function that has the TM_ANNOTATE attribute
///   -- Any function that is passed to TM_EXECUTE_C
///   -- Any lambda with an TM_OPAQUE* parameter
///   -- Any annotated constructor
///   -- Any function with an annotation that begins with TM_RENAME_
///   -- Any function that reachable from any of the above
///   -- Note: there is an exemption for functions annotated with TM_PURE
///
/// - Cloning of instrumentable functions
///   -- This produces a new version of each function, renamed and ready to be
///      transformed
///   -- In the case of TM_PURE, we do not clone
///   -- In the case of TM_RENAME_, we instrument the original and do not clone
///
/// - Transformation of cloned functions
///   -- Loads become function calls
///   -- Stores become function calls
///   -- Accesses to volatiles and atomics, as well as unsafe instructions, must
///      be dominated by a call to TM_UNSAFE
///   -- Calls to special functions get auto-remapped (e.g., malloc, free)
///   -- Calls to other functions get remapped to clones if we know the clone
///      exists
///   -- Indirect calls and calls to functions for which we cannot find a clone
///      make use of run-time clone lookup
///
/// - Transformation of boundaries of instrumented regions
///   -- lambdas with TM_OPAQUE* argument get some extra control flow
///   -- calls to TM_EXECUTE_C must be transformed so they take both
///      the name of a function, and the name of its clone, unless the clone
///      cannot be found
///
/// - Optimizations
///   -- Elimination of calls to TM_UNSAFE that are dominated by calls to
///      TM_UNSAFE
///
/// - Run-Time Mapping
///   -- Creation of initialization code to produce run-time-accessible mappings
///      from functions to their clones.
class tm_plugin : public llvm::ModulePass {
  /// Every pass must have an ID field
  static char ID;

  /// All of the signatures for types and functions that we use throughout the
  /// plugin
  signatures sigs;

  /// a worklist for helping us to search for functions during discovery
  std::queue<llvm::Function *> worklist;

  /// a worklist for helping us to remove calls to TM_CTOR during discovery
  llvm::SmallVector<llvm::Instruction *, 128> ctor_list;

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

  /// Discovery: Find the constructors, and put them in the worklist
  void discover_constructor(llvm::Module &M);

  /// Discovery: Process the worklist until it is empty, in order to find all
  /// reachable functions.
  void discover_reachable_funcs();

  /// Cloning: clone all functions in the worklist
  void create_clones();

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

  /// Optimization: Remove extra unsafe function calls from a basic block
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