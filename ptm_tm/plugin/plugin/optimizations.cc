#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "../../common/tm_defines.h"

#include "tm_plugin.h"

using namespace llvm;

/// Process the module to find situations in which a call to TM_UNSAFE is
/// dominated by another call to TM_UNSAFE.  When such a situation is found,
/// remove the second call.
void tm_plugin::optimize_unsafe(llvm::Module &M) {
  /// a worklist for helping us to search for dead function calls
  llvm::SmallVector<llvm::Instruction *, 128> worklist;

  // Iterate through the instructions of each function in the module
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {
    for (auto bb = fn->begin(), bbe = fn->end(); bb != bbe; ++bb) {
      // track if we have already found a call to TM_UNSAFE *in this block*
      bool unsafe_flag = false;
      for (auto I = bb->begin(), E = bb->end(); I != E; ++I) {
        if (isa<CallInst>(*I)) {
          if (Function *Callee = cast<CallInst>(*I).getCalledFunction()) {
            if (Callee->getName() == TM_UNSAFE_STR) {
              if (unsafe_flag) {
                worklist.push_back(&*I);
              } else {
                unsafe_flag = true;
              }
            }
          }
        }
      }
    }
  }

  // Remove dead unsafe calls
  while (!worklist.empty()) {
    auto *I = worklist.pop_back_val();
    I->eraseFromParent();
  }
}
