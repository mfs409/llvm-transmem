#include <cstdlib>

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "../../common/tm_defines.h"

#include "tm_plugin.h"

using namespace llvm;

/// Transform the body of each clone we created, so that it calls instrumented
/// functions and uses instrumentation on loads and stores.
void tm_plugin::instrument_function_bodies() {
  // Iterate over each basic block of each function that has a clone.  We must
  // iterate over basic blocks, instead of over the function's instructions
  // directly, in order to use ReplaceInstWithValue.  Within the BB, iterate
  // over all instructions.
  for (auto func : functions) {
    Function *clone = func.second.clone;
    if (std::find(purelist.begin(), purelist.end(), clone) != purelist.end()) {
      continue;
    }
    for (auto bb = clone->begin(), bbe = clone->end(); bb != bbe; ++bb) {
      for (auto inst = bb->begin(), E = bb->end(); inst != E;) {
        CallSite callsite(cast<Value>(inst));
        if (callsite) {
          // If transform_callsite returns an instruction, then we should
          // use that instruction instead of the one we had
          if (Instruction *new_inst = transform_callsite(callsite, inst)) {
            ReplaceInstWithValue(bb->getInstList(), inst, new_inst);
            inst = BasicBlock::iterator(new_inst); // update iterator
          }
        }

        // If this is an atomic/volatile RMW memory access, or a fence, insert
        // TM_UNSAFE before it
        else if (isa<AtomicRMWInst>(inst) || isa<AtomicCmpXchgInst>(inst)) {
          prefix_with_unsafe(inst);
        }

        // If this is a store instruction, either convert it to a function call
        // or treat it as unsupported (conversion returns nullptr for
        // atomic/volatile stores)
        else if (isa<StoreInst>(inst)) {
          StoreInst *store = dyn_cast<StoreInst>(&*inst);
          if (CallInst *new_store = convert_store(store)) {
            ReplaceInstWithInst(store, new_store);
            inst = BasicBlock::iterator(new_store); // update iterator
          } else {
            prefix_with_unsafe(inst);
          }
        }

        // If this is a load instruction, either convert it to a function call
        // or treat it as unsupported (conversion returns nullptr for
        // atomic/volatile loads)
        else if (isa<LoadInst>(inst)) {
          LoadInst *load = dyn_cast<LoadInst>(&*inst);
          // The normal behavior is to use convert_load to instrument this load.
          // If convert_load returns nullptr, then we need to prefix with
          // unsafe, because it's a volatile or atomic, which is not supported.
          if (instrument_reads) {
            if (Instruction *new_load = convert_load(load)) {
              ReplaceInstWithInst(load, new_load);
              inst = BasicBlock::iterator(new_load);
            } else {
              prefix_with_unsafe(inst);
            }
          }
          // We don't expect to have volatile or atomic accesses within
          // transactions, but just to be safe, we're not going to just say
          // "hey, it's a load, don't instrument".  Instead, we will say "if
          // it's a /regular/ load, then don't instrument.  If it's volatile or
          // atomic, then do as we would with a volatile or atomic store."
          else {
            if (load->isVolatile() || load->isAtomic()) {
              prefix_with_unsafe(inst);
            }
          }
        }

        // For other llvm instructions, we just ignore them because they do not
        // access memory

        // Terminator operations
        else if (isa<ReturnInst>(inst) || isa<BranchInst>(inst) ||
                 isa<SwitchInst>(inst) || isa<IndirectBrInst>(inst) ||
                 isa<ResumeInst>(inst) || isa<CatchSwitchInst>(inst) ||
                 isa<CatchReturnInst>(inst) || isa<CleanupReturnInst>(inst) ||
                 isa<UnreachableInst>(inst)) {
        }

        // Binary operations are safe to ignore
        // Bitwise binary operations are subset of binary operations
        else if (isa<BinaryOperator>(inst)) {
          // Register to register operations
        }

        // Vector operations
        else if (isa<ShuffleVectorInst>(inst) || isa<InsertElementInst>(inst) ||
                 isa<ExtractElementInst>(inst)) {
        }

        // Unary operations
        else if (isa<UnaryInstruction>(inst)) {
        }

        // Aggregate operations
        else if (isa<InsertValueInst>(inst) || isa<ExtractValueInst>(inst)) {
        }

        // Addressing operations
        else if (isa<GetElementPtrInst>(inst) || isa<FenceInst>(inst) ||
                 isa<AllocaInst>(inst)) {
        }

        // Conversion operations
        else if (isa<TruncInst>(inst) || isa<FPTruncInst>(inst) ||
                 isa<SExtInst>(inst) || isa<ZExtInst>(inst) ||
                 isa<FPExtInst>(inst) || isa<PtrToIntInst>(inst) ||
                 isa<IntToPtrInst>(inst) || isa<FPToUIInst>(inst) ||
                 isa<FPToSIInst>(inst) || isa<UIToFPInst>(inst) ||
                 isa<SIToFPInst>(inst) || isa<BitCastInst>(inst) ||
                 isa<AddrSpaceCastInst>(inst)) {
        }

        // Other safe operations
        else if (isa<ICmpInst>(inst) || isa<FCmpInst>(inst) ||
                 isa<PHINode>(inst) || isa<SelectInst>(inst) ||
                 isa<VAArgInst>(inst) || isa<LandingPadInst>(inst) ||
                 isa<CatchPadInst>(inst) || isa<CleanupPadInst>(inst)) {
        }
        // Unknown / unsupported
        else {
          exit(EXIT_FAILURE);
        }
        inst++;
      }
    }
  }
}

/// Given a call site, attempt to create a call to the clone of the function
/// that is called by the call site, or a call through the TM_TRANSLATE
/// mechanism.  This may insert several instructions into the basic block, but
/// will always return a CallInst that can be passed to ReplaceInstWithValue to
/// replace @param inst.
Instruction *tm_plugin::transform_callsite(CallSite &callsite,
                                           BasicBlock::iterator inst) {
  // For inline assembly, serialize the transaction
  if (const CallInst *CI = dyn_cast<CallInst>(inst)) {
    if (CI->isInlineAsm()) {
      prefix_with_unsafe(inst);
      return nullptr;
    }
  }

  // If the called function is indirect, use the TM_TRANSLATE infrastructure.
  //
  // WARNING: this is not tested for indirect calls within a try block
  Function *callee = callsite.getCalledFunction();
  if (!callee) {
    if (callsite.isIndirectCall()) {
      // Put everything *before* ins_pt, so the caller can replace ins_pt
      Instruction *ins_pt = dyn_cast<Instruction>(&*inst);
      // Turn the original function into a void*
      Value *orig = callsite.getCalledValue();
      BitCastInst *erased =
          new BitCastInst(orig, sigs.get_type(signatures::OPAQUE), "", ins_pt);
      // call TM_TRANSLATE, then cast the result back to the original func type
      CallInst *xlate = CallInst::Create(sigs.get_func(signatures::TRANSLATE),
                                         {erased}, "", ins_pt);
      BitCastInst *updated =
          new BitCastInst(xlate, orig->getType(), "", ins_pt);
      return create_callinst(callsite, inst, updated, orig);
    }
    // WARNING: this fallthrough code is not tested.  If we wind up here, we
    // will call the original, uninstrumented function
    return nullptr;
  }

  // If the clone version is in purelist, return the original version
  if (std::find(purelist.begin(), purelist.end(), callee) != purelist.end()) {
    return nullptr;
  }

  // WARNING: for now, we serialize on any exception in a transaction, even one
  // that gets caught before commit
  if (callee->getName() == "__cxa_allocate_exception" ||
      callee->getName() == "__cxa_free_exception" ||
      callee->getName() == "__cxa_throw" ||
      callee->getName() == "__cxa_begin_catch" ||
      callee->getName() == "__cxa_end_catch" ||
      callee->getName() == "__cxa_rethrow") {
    prefix_with_unsafe(inst);
    return nullptr;
  }

  // We ignore calls to TM_COMMIT_HANDLER, since they are to the TM API
  if (callee->getName() == TM_COMMIT_HANDLER_STR) {
    return nullptr;
  }

  // Try to find the clone of the called function.  Normally, we get it from the
  // clone list.  However, we hard-code malloc, free, memcpy, memset and
  // memmove, since they redirect to the TM library.
  //
  // IF this istruction is intrinsic, check whether it is a safe call
  // or an unsafe call in a transaction
  Function *clone = nullptr;
  if (callee->getName() == "malloc") {
    clone = sigs.get_func(signatures::MALLOC);
  } else if (callee->getName() == "aligned_alloc") {
    clone = sigs.get_func(signatures::ALIGNED_ALLOC);
  } else if (callee->getName() == "free") {
    clone = sigs.get_func(signatures::FREE);
  } else if (callee->getName() == "llvm.memcpy.p0i8.p0i8.i64") {
    clone = sigs.get_func(signatures::MEMCPY);
  } else if (callee->getName() == "llvm.memset.p0i8.i64") {
    clone = sigs.get_func(signatures::MEMSET);
  } else if (callee->getName() == "llvm.memmove.p0i8.p0i8.i64") {
    clone = sigs.get_func(signatures::MEMMOVE);
  } else if (callee->isIntrinsic()) {
    convert_intrinsics(callee, inst);
    return nullptr;
  } else {
    clone = get_clone(callee);
  }

  // If there's no clone in this TU, fall back to TM_TRANSLATE.  This code is
  // mostly like the indirect call code above, but it handles invoke a bit more
  // carefully.
  if (!clone) {
    Instruction *ins_pt = dyn_cast<Instruction>(&*inst);
    Function *orig = callsite.getCalledFunction();
    BitCastInst *erased =
        new BitCastInst(orig, sigs.get_type(signatures::OPAQUE), "", ins_pt);
    CallInst *xlate = CallInst::Create(sigs.get_func(signatures::TRANSLATE),
                                       {erased}, "", ins_pt);
    BitCastInst *updated = new BitCastInst(xlate, orig->getType(), "", ins_pt);
    if (callsite.isInvoke())
      return create_invokeinst(callsite, inst, updated, orig);
    else
      return create_callinst(callsite, inst, updated, orig);
  }

  // If we found a clone, create a call or invoke using the clone
  if (CallInst *callinst = dyn_cast<CallInst>(inst)) {
    return create_callinst(callsite, inst, clone, clone);
  } else if (InvokeInst *invokeinst = dyn_cast<InvokeInst>(inst)) {
    return create_invokeinst(callsite, inst, clone, clone);
  }

  // WARNING: this code path is not tested.  It seems dangerous to return a
  //          nullptr in this case.
  return nullptr;
}

/// Replace a call instruction to the original code with a call to something
/// safe
Instruction *tm_plugin::create_callinst(CallSite &callsite,
                                        BasicBlock::iterator inst, Value *val,
                                        Value *orig_val) {
  CallInst *new_call = CallInst::Create(
      val, SmallVector<Value *, 8>(callsite.arg_begin(), callsite.arg_end()),
      "", dyn_cast<Instruction>(&*inst));
  // If it's an indirect call, use the calling conventions of the original
  if (!callsite.isIndirectCall())
    new_call->setCallingConv(dyn_cast<Function>(orig_val)->getCallingConv());
  if (!new_call->getDebugLoc())
    new_call->setDebugLoc(inst->getDebugLoc());
  return new_call;
}

/// Replace an invoke instruction to the original code with a call to something
/// safe
Instruction *tm_plugin::create_invokeinst(CallSite &callsite,
                                          BasicBlock::iterator inst, Value *val,
                                          Value *orig_val) {
  // build a new invoke, using the landing pad and calling conventions of the
  // old invoke
  InvokeInst *invokeinst = dyn_cast<InvokeInst>(inst);
  InvokeInst *newinst = InvokeInst::Create(
      val, invokeinst->getNormalDest(), invokeinst->getUnwindDest(),
      SmallVector<Value *, 8>(callsite.arg_begin(), callsite.arg_end()), "",
      invokeinst);
  newinst->setCallingConv(dyn_cast<Function>(orig_val)->getCallingConv());
  if (!newinst->getDebugLoc())
    newinst->setDebugLoc(inst->getDebugLoc());
  return newinst;
}

/// Insert a call to TM_UNSAFE before the current instruction
void tm_plugin::prefix_with_unsafe(BasicBlock::iterator inst) {
  CallInst *new_call = CallInst::Create(sigs.get_func(signatures::UNSAFE), {},
                                        "", dyn_cast<Instruction>(&*inst));
}

/// Given a store, either replace it with a call to TM_STORE, or return
/// nullptr to indicate that the store is not TM-safe
CallInst *tm_plugin::convert_store(StoreInst *store) {
  if (store->isVolatile() || store->isAtomic())
    return nullptr;
  // Get pointer and value operands of the store, and the base type
  Value *ptr = store->getPointerOperand();
  Value *val = store->getValueOperand();
  Type *type = ptr->getType()->getPointerElementType();

  // If the type is unknown return nullptr
  if (!sigs.get_store(val->getType()))
    return nullptr;

  // If it's not a pointer type, then we just make a replacement call
  // instruction
  if (!type->isPointerTy())
    return CallInst::Create(sigs.get_store(val->getType()), {val, ptr}, "");

  // It's a pointer type.  We need to convert the arguments from type* and
  // type** to void* and void** (by adding two bitcasts), and then we can make
  // the replacement call instruction
  //
  // NB: both bitcasts go before the original store
  BitCastInst *VS =
      new BitCastInst(val, sigs.get_type(signatures::OPAQUE), "", store);
  BitCastInst *VSS =
      new BitCastInst(ptr, sigs.get_type(signatures::PTR_OPAQUE), "", store);
  return CallInst::Create(sigs.get_store(val->getType()), {VS, VSS}, "");
}

/// Given a load, either replace it with a call to TM_LOAD, or return nullptr to
/// indicate that the load is not TM-safe
Instruction *tm_plugin::convert_load(LoadInst *load) {
  if (load->isVolatile() || load->isAtomic())
    return nullptr;
  // Get pointer and return type of the load
  Value *ptr = load->getPointerOperand();
  Type *type = ptr->getType()->getPointerElementType();

  // If the type is unknown reurn nullptr
  if (!sigs.get_load(type))
    return nullptr;

  // If it's not a pointer type, then we just make a replacement instruction
  if (!type->isPointerTy())
    return CallInst::Create(sigs.get_load(type), {ptr}, "");

  // It's a pointer type.  We need to convert the argument from type** to
  // void**, by adding a bitcast, and then we need to bitcast the return value
  // of the call from void* to type*.  Then we return the bitcast instruction,
  // so that the caller can replace load with it
  //
  // NB: first bitcast and new load go before the original load
  BitCastInst *VSS =
      new BitCastInst(ptr, sigs.get_type(signatures::PTR_OPAQUE), "", load);
  CallInst *new_load = CallInst::Create(sigs.get_load(type), {VSS}, "", load);
  return new BitCastInst(new_load, type);
}

/// Handle LLVM intrinsic instructions specially, since some are supported and
/// others require serialization (but should not happen in C++ code?)
void tm_plugin::convert_intrinsics(llvm::Function *callee,
                                   BasicBlock::iterator inst) {
  // These are unsafe llvm intrinsics, insert TM_UNSAFE before it
  //          - Some of Code Generator Intrinsics
  //          - Trampoline Intrinsics
  //          - Some of General Intrinsics
  //          - Element Wise Atomic Memory Intrinsics
  //
  // WARNING: for other llvm intrinsics, we just ignore them for now.  Based on
  //          prior analysis, this is OK, but if the LLVM IR grows, we need to
  //          revisit this.
  if (callee->getName() == "llvm.clear_cache" ||
      callee->getName() == "llvm.init.trampoline" ||
      callee->getName() == "llvm.adjust.trampoline" ||
      callee->getName() == "llvm.trap" ||
      callee->getName() == "llvm.debugtrap" ||
      callee->getName() ==
          "llvm.memcpy.element.unordered.atomic.p0i8.p0i8.i64" ||
      callee->getName() ==
          "llvm.memmove.element.unordered.atomic.p0i8.p0i8.i64" ||
      callee->getName() == "llvm.memset.element.unordered.atomic.p0i8.i64" ||
      callee->getName().startswith("llvm.load.relative.")) {
    prefix_with_unsafe(inst);
  }
  // WARNING: The LLVM optimizer translates a masked intrinsic like
  //          llvm.masked.gather or llvm.masked.scatter to a chain of basic
  //          blocks, and loads elements one-by-one if the appropriate mask bit
  //          is set. Therefore, we could not discover these intrinsics in
  //          bitcode, so for now we just insert UNSAFE before them
  if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(inst)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::masked_load:
    case Intrinsic::masked_store:
    case Intrinsic::masked_gather:
    case Intrinsic::masked_scatter:
      prefix_with_unsafe(inst);
    }
  }
}
