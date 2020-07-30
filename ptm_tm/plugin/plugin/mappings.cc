#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "../../common/tm_defines.h"

#include "tm_plugin.h"

using namespace llvm;

/// Create a function that contains calls to TM_REG_CLONE for each of the
/// clones that exists in this module, and then add that function to the list of
/// module constructors, so that it will run automatically when the program
/// starts
///
/// WARNING: we don't currently put a destructor into the module.  When we begin
///          supporting loading/unloading .so files, we will need to add it.
void tm_plugin::create_runtime_mappings(llvm::Module &M) {
  // Create a Function* to represent the TM_REG_CLONE function. Its signature is
  // void(*)(void*, void*).
  Function *reg_clone =
      cast<Function>(M.getOrInsertFunction(
                          TM_REG_CLONE_STR,
                          FunctionType::get(sigs.get_type(signatures::VOID),
                                            {sigs.get_type(signatures::OPAQUE),
                                             sigs.get_type(signatures::OPAQUE)},
                                            false))
                         .getCallee());

  // Create a function that will make the calls to TM_REG_CLONE repeatedly to
  // register the entries in clones.  Its signature is void(*)(void).
  Function *static_initializer = Function::Create(
      FunctionType::get(sigs.get_type(signatures::VOID), {}, false),
      GlobalValue::LinkageTypes::InternalLinkage, TM_STATIC_INITIALIZER_STR,
      &M);

  // Iterate through the list of clones and use it to insert calls to
  // reg_clone() into static_initializer().
  BasicBlock *calls =
      BasicBlock::Create(M.getContext(), "", static_initializer);
  IRBuilder<> builder(calls);
  for (auto b = functions.begin(), e = functions.end(); b != e; ++b) {
    auto from = builder.CreateBitCast(b->second.orig,
                                      sigs.get_type(signatures::OPAQUE));
    auto to = builder.CreateBitCast(b->second.clone,
                                    sigs.get_type(signatures::OPAQUE));
    builder.CreateCall(reg_clone, {from, to});
  }

  // Finish the body by adding a void return statement.
  builder.CreateRetVoid();

  // Insert our function into the global ctors list for this module
  // NB: the priority shouldn't matter
  llvm::appendToGlobalCtors(M, static_initializer, 65535);
}