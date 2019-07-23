#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "../include/tm_api.h"

#include "local_config.h"
#include "tm_plugin.h"

using namespace llvm;

/// Iterate through all of the annotations in the Module, and whenever we find
/// an annotation that ought to be associated with a Function, we get the
/// corresponding Function object from the Module and attach the annotation
/// directly to it.
void tm_plugin::attach_annotations_to_functions(Module &M) {
  // Get the global variable that holds all of the annotations
  GlobalVariable *annotations = M.getNamedGlobal("llvm.global.annotations");

  // If there are no annotations, we're done and we didn't change anything
  if (!annotations) {
    return;
  }

  // Go through the annotations, and for each that is associated with a
  // function, attach it to the function
  auto a = cast<ConstantArray>(annotations->getOperand(0));
  for (size_t i = 0; i < a->getNumOperands(); i++) {
    auto e = cast<ConstantStruct>(a->getOperand(i));
    if (Function *f = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
      auto anno = cast<ConstantDataArray>(
                      cast<GlobalVariable>(e->getOperand(1)->getOperand(0))
                          ->getOperand(0))
                      ->getAsCString();
      f->addFnAttr(anno);
    }
  }
}

/// Find the functions in the module that have TM_ANNOTATE, TM_PURE, or
/// TM_RENAME annotations, and put them in the work list
void tm_plugin::discover_annotated_funcs(Module &M) {
  // Before iterating through functions, see if there are any instances of
  // TM_EXECUTE, TM_EXECUTE_C, or TM_EXECUTE_C_INTERNAL.  If so, add them to the
  // pure list.  Also do _ZNSt14_Function_baseD2Ev, so that nested lambdas don't
  // serialize
  if (Function *f = M.getFunction(TM_EXECUTE_STR)) {
    purelist.push_back(dyn_cast<Function>(f));
  }
  if (Function *f = M.getFunction(TM_EXECUTE_C_STR)) {
    purelist.push_back(dyn_cast<Function>(f));
  }
  if (Function *f = M.getFunction(TM_EXECUTE_C_INTERNAL_STR)) {
    purelist.push_back(dyn_cast<Function>(f));
  }
  if (Function *f = M.getFunction("_ZNSt14_Function_baseD2Ev")) {
    purelist.push_back(dyn_cast<Function>(f));
  }

  // populate the pure list with any additional custom overrides
  for (const char *name : discovery_pure_overrides) {
    if (Function *f = M.getFunction(name))
      purelist.push_back(dyn_cast<Function>(f));
  }

  // Go through the functions, searching for annotated definitions.  Note that
  // annotations can only be attached to definitions, not declarations.
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {
    if (!(fn->isDeclaration())) {
      if (fn->hasFnAttribute(TM_ANNOTATE_STR)) {
        worklist.push(dyn_cast<Function>(fn));
      } else if (fn->hasFnAttribute(TM_PURE_STR)) {
        purelist.push_back(dyn_cast<Function>(fn));
      }

      // Find functions with the TM_RENAME attribute, change their names, and
      // put them in the work list.  We assume that these functions have
      // TM_RENAME as attribute #1
      auto FnAttr = fn->getAttributes().getFnAttributes();
      auto AttrSTR = FnAttr.getAsString(1);
      auto begin_pos = AttrSTR.find(TM_RENAME_STR);
      if (begin_pos != StringRef::npos) {
        auto end_pos = AttrSTR.find("\"", begin_pos);
        if (end_pos != StringRef::npos && end_pos != begin_pos) {
          fn->setName(Twine(
              TM_PREFIX_STR,
              AttrSTR.substr(begin_pos + strlen(TM_RENAME_STR),
                             end_pos - (begin_pos + strlen(TM_RENAME_STR)))));
          worklist.push(dyn_cast<Function>(fn));
          // Find the original version of functions that are being renamed, and
          // put them into the rename list
          Function *func =
              M.getFunction(fn->getName().substr(strlen(TM_PREFIX_STR)));
          renamelist.insert({dyn_cast<Function>(fn), func});
        }
      }
    }
  }
}

/// Find the functions in the module that are actually lambda operator()s that
/// conform to our API, and add them to the work list
void tm_plugin::discover_lambda_funcs(Module &M) {
  // Rather than try to construct the special type we use for identifying
  // lambdas, we will check if the type is already in the module... if not, then
  // we can return early
  StructType *OPAQUE = M.getTypeByName("struct." TM_OPAQUE_STR);
  if (!OPAQUE) {
    return;
  }
  PointerType *PTR_OPAQUE = PointerType::getUnqual(OPAQUE);

  // Iterate through the functions of the module to do the search
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {

    // Get the number of arguments to the function, and track if any argument is
    // of type PTR_OPAQUE
    //
    // WARNING: we aren't paying attention to argument position
    int arg_count = 0, found = 0;
    if (!(fn->isDeclaration())) {
      for (auto I = fn->arg_begin(), E = fn->arg_end(); I != E; ++I) {
        arg_count++;
        Argument &argv = *I;
        if (argv.getType() == PTR_OPAQUE)
          found++;
      }
    }

    // We assume that if found == 1 and arg_count == 2, then we have found one
    // of the lambdas that we created through our API, and we should save it so
    // we can do more with it later it.  However, we don't want to clone the
    // generic instance of
    // std::function<void (TM_OPAQUE*)>::operator()(TM_OPAQUE*) const, only
    // the lambdas with that same signature.
    //
    // TODO: we are explicitly skipping std::function<>::operator()(...).  It
    //       seems not to be necessary, so then why is it in the .o file?
    //
    // WARNING: the "9" in the mangle assumes the length of TM_OPAQUE_STR
    if (found == 1 && arg_count == 2 &&
        fn->getName() != ("_ZNKSt8functionIFvP9" TM_OPAQUE_STR "EEclES1_")) {
      worklist.push(dyn_cast<Function>(fn));
      lambdas.push_back(dyn_cast<Function>(fn));
    }
  }
}

/// Find the functions in the module that are called via TM_EXECUTE_C, and put
/// them in the work list
void tm_plugin::discover_capi_funcs(Module &M) {
  // Find any call to TM_EXECUTE_C in any function's body
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {
    for (inst_iterator I = inst_begin(*fn), E = inst_end(*fn); I != E; ++I) {
      CallSite CS(cast<Value>(&*I)); // CallSite is CallInst | InvokeInst
      if (CS) {
        if (Function *Callee = CS.getCalledFunction()) {
          // If this is a call to TM_EXECUTE_C, then operand 1 is a function
          // that needs to be processed, unless (a) it's a function pointer
          // that we don't know how to turn into a function, or (b) the
          // function we're calling does not have a definition in this TU
          if (Callee->getName() == TM_EXECUTE_C_STR) {
            if (Function *f = dyn_cast<Function>(CS.getArgOperand(1))) {
              if (!f->isDeclaration()) {
                worklist.push(f);
              }
            }
          }
        }
      }
    }
  }
}

/// Find annotated constructors, and put them in the work list
void tm_plugin::discover_constructor(Module &M) {
  // Find any call to TM_CTOR in any function's body
  for (auto fn = M.getFunctionList().begin(), e = M.getFunctionList().end();
       fn != e; ++fn) {
    for (inst_iterator I = inst_begin(*fn), E = inst_end(*fn); I != E; ++I) {
      CallSite CS(cast<Value>(&*I)); // CallSite is CallInst | InvokeInst
      if (CS) {
        if (Function *Callee = CS.getCalledFunction()) {
          if (Callee->getName() == TM_CTOR_STR) {
            if (Function *parent = CS.getCaller()) {
              if (!parent->isDeclaration()) {
                worklist.push(parent);
                ctor_list.push_back(&*I);
              }
            }
          }
        }
      }
    }
  }

  // Remove TM_CTOR calls
  while (!ctor_list.empty()) {
    auto *I = ctor_list.pop_back_val();
    I->eraseFromParent();
  }
}

/// Process the worklist until we are sure that there are no TM-reachable
/// functions that we have not found.  We want to find functions that are in
/// deep call chains, but we can only handle call chains that are entirely
/// within a Module.  Inter-module calls are not handled efficiently by our TM
/// plugin, because annotations are not part of the type system.  Thus
/// inter-module calls will lead to either (a) dynamic lookup, or (b)
/// serialization.
void tm_plugin::discover_reachable_funcs() {
  // Iterate over the annotated function queue to find all functions that
  // might need to be visited
  while (!worklist.empty()) {
    auto fn = worklist.front();
    worklist.pop();
    // If the current function is one we haven't seen before, save it and
    // process it
    if (functions.find(fn) == functions.end()) {
      // Track if the function is a lambda
      function_features f;
      f.orig = fn;
      if (std::find(lambdas.begin(), lambdas.end(), fn) != lambdas.end())
        f.orig_lambda = true;
      // If pure, clone == self; if TM_RENAME, clone \in renamelist; else delay
      // on making clone
      f.clone = nullptr;
      if (std::find(purelist.begin(), purelist.end(), fn) != purelist.end())
        f.clone = fn;
      auto origFn = renamelist.find(fn);
      if (origFn != renamelist.end()) {
        f.orig = (*origFn).second;
        f.clone = (*origFn).first;
        functions.insert({(*origFn).second, f});
      } else {
        functions.insert({fn, f});
      }
      // process this function by finding all of its call instructions and
      // adding them to the worklist
      for (inst_iterator I = inst_begin(*fn), E = inst_end(*fn); I != E; ++I) {
        // CallSite is CallInst | InvokeInst | Indirect call
        CallSite CS(cast<Value>(&*I));
        if (CS)
          // Get the function, figure out if we have a body for it.  If indirect
          // call, we'll get a nullptr.
          if (Function *func = CS.getCalledFunction())
            if (!(func->isDeclaration()))
              worklist.push(func);
      }
    }
  }
}

/// Iterate through the list of discovered functions, and for each one, generate
/// a clone and add it to the module
///
/// NB: The interaction between this code and C++ name mangling is not what one
///     would expect.  If a mangled name is _Z18test_clone_noparamv, the clone
///     name will be tm__Z18test_clone_noparamv, not _Z21tm_test_clone_noparamv.
///     This appears to be a necessary cost of memory instrumentation without
///     front-end support
void tm_plugin::create_clones() {
  for (auto &fn : functions) {
    // If a function is a special function or it has a pure attribute,
    // there is no need to generate a clone version
    if (renamelist.find(fn.second.clone) == renamelist.end() &&
        std::find(purelist.begin(), purelist.end(), fn.first) ==
            purelist.end()) {

      // Sometimes, when cloning one function into another through the standard
      // LLVM utilities, the new function will have more arguments, or arguments
      // in a different order, than the original function.  Consequently,
      // CloneFunction takes a ValueToValueMap that explains how the arguments
      // to the original map to arguments of the clone. In our case, we are not
      // modifying the argument order or count, so an empty map is sufficient.
      ValueToValueMapTy v2vmap;

      // Create the clone.  We are using the simplest cloning technique that
      // LLVM offers.  The clone will go into the current module
      Function *newfunc = CloneFunction(fn.first, v2vmap, nullptr);

      // Give the new function a name by concatenating the TM_PREFIX_STR with
      // the original name of the function, and then save the new function so we
      // can instrument it in a later phase
      newfunc->setName(Twine(TM_PREFIX_STR, fn.first->getName()));
      fn.second.clone = newfunc;
    }
  }
}