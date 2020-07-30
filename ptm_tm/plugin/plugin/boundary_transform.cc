#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "../../common/tm_defines.h"

#include "tm_plugin.h"

using namespace llvm;

/// Handle conversion of transaction region begin/end for C API
void tm_plugin::convert_region_begin_c_api(llvm::Module &M) {
  // Iterate through the instructions of each function in the module
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {
    for (BasicBlock &B : (*fn)) {
      BasicBlock::iterator I = B.begin();
      while (I != B.end()) {
        // If we find a callsite to TM_EXECUTE_C, we can translate it
        CallSite CS(cast<Value>(I));
        if (CS) {
          if (Function *Callee = CS.getCalledFunction()) {
            if (Callee->getName() == TM_EXECUTE_C_STR) {
              // Find the called function (arg 1) and the clone version of it.
              //
              // NB: If we get a function pointer, then we don't transform the
              //     call.  It's on the TM library to do a dynamic translation
              //     in this case.
              if (Function *Orig = dyn_cast<Function>(CS.getArgOperand(1))) {
                if (Function *ClonedFunc = get_clone(Orig)) {
                  // Create the replacement call instruction... it can be a call
                  // or an invoke
                  SmallVector<Value *, 3> Args(CS.arg_begin() + 1,
                                               CS.arg_end());
                  Args.push_back(dyn_cast<Value>(ClonedFunc));
                  Instruction *NewCI;
                  if (CS.isInvoke()) {
                    InvokeInst *invokeinst = dyn_cast<InvokeInst>(I);
                    NewCI = InvokeInst::Create(sigs.get_func(signatures::CAPI),
                                               invokeinst->getNormalDest(),
                                               invokeinst->getUnwindDest(),
                                               Args, "");
                  } else {
                    NewCI = CallInst::Create(sigs.get_func(signatures::CAPI),
                                             Args, "");
                  }
                  // Replace the instruction, and update the iterator
                  ReplaceInstWithInst(dyn_cast<Instruction>(&*I), NewCI);
                  I = BasicBlock::iterator(NewCI);
                }
              }
            }
          }
        }
        I++;
      }
    }
  }
}

/// Modify the body of any lambda that is going to be called, so that it has a
/// mechanism for calling its clone instead.  Note that these bodies are always
/// going to be transactional-only, so we don't need to worry about calls from a
/// nontransactional context.
///
/// NB: At -O3 optimization, the clone ends up inlining into the lambda :)
void tm_plugin::convert_lambdas_cxx_api(Module &M) {
  // We do this for every lambda that we identified before:
  for (auto i : lambdas) {
    // WARNING: If we don't have a clone, we fail silently.  That's probably not
    //          the right behavior, but we think that there will always be a
    //          clone.  If not, should we just insert a call to UNSAFE?
    auto clone = functions.find(i);
    if (clone == functions.end()) {
      continue;
    }

    // We need the args to the function, so we can use them to call the clone
    Function *fn = i;
    Function::arg_iterator args = fn->arg_begin();
    Value *obj = dyn_cast<Value>(args++);
    Value *opaque = dyn_cast<Value>(args++); // expected to be TM_OPAQUE

    // We also need a nullptr value of type TM_OPAQUE
    Type *opaque_type = opaque->getType();
    Constant *opaque_null = Constant::getNullValue(opaque_type);

    // Get the first basic block of the function.  We put everything before it
    Function::iterator BBI = fn->begin();
    BasicBlock *BB = dyn_cast<BasicBlock>(BBI);

    // Create two basic blocks.  The first is the new entry to the function, and
    // holds a condition.  The second is for if the condition is true.  The BB
    // from above will be the 'false' target of the compare
    BasicBlock *compare = BasicBlock::Create(M.getContext(), "", fn, BB);
    BasicBlock *iftrue = BasicBlock::Create(M.getContext(), "", fn, BB);

    // Build the comparison block: if the opaque pointer is not null, go to
    // iftrue, else to BB
    IRBuilder<> builder(compare);
    Value *NEQ = builder.CreateICmpNE(opaque, opaque_null);
    builder.CreateCondBr(NEQ, iftrue, BB);

    // Build the "if true" block... it just calls the clone and returns
    builder.SetInsertPoint(iftrue);
    builder.CreateCall(clone->second.clone, {obj, opaque});
    builder.CreateRetVoid();
  }
}