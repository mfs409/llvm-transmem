#include <algorithm>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "../../common/tm_defines.h"

#include "tm_plugin.h"

using namespace llvm;
using namespace std;

namespace {

/// In the RAII API, transactions are delineated by a special constructor (ctor)
/// and destructor (dtor).  We will ultimately need to instrument the sub-graph
/// of the CFG that starts with the instruction immediately succeeding the ctor,
/// and ends with the instruction immediately preceding the dtor.
///
/// The complicating factors are that (1) the ctor and dtor could be at *any*
/// point within the CFG, and (2) transactions can be nested.
///
/// To keep things simple, we split BBs that have ctors and dtors.  The goal is
/// that each ctor is in its own BB, and each dtor is the first instruction in
/// its BB.  Organizing the BBs like this gives us a nice invariant: *every*
/// instruction in the graph of blocks between the ctor and dtor is supposed to
/// be instrumented.
///
/// @returns A vector of all the ctors in the module.  Note that while we return
///          a vector of raii_region_t structs, those structs are incomplete:
///          they only have the begin instructions.
vector<raii_region_t> normalize_raii_boundaries(Module &M) {
  // A collection of all the constructors we found
  vector<raii_region_t> ctors;

  // While traversing the BBs of a module, we do not want to modify the CFG, so
  // whenever we find an instruction that should cause a BB to be split, we put
  // it here, and we wait to do all splitting until the end.
  vector<Instruction *> split_points;

  // Traverse each basic block of each function
  for (auto fn = M.getFunctionList().begin(), fe = M.getFunctionList().end();
       fn != fe; ++fn) {
    for (auto bb = fn->getBasicBlockList().begin(),
              be = fn->getBasicBlockList().end();
         bb != be; ++bb) {
      // When we see a ctor, we can't just say "make three BBs" here, because
      // the ctor might already be the last instruction of the BB.  This
      // variable is for tracking if the preceding instruction was a ctor.
      bool prev_inst_was_raii_start = false;
      for (auto ii = bb->begin(), ie = bb->end(); ii != ie; ii++) {
        // make sure we don't push an instruction twice
        bool already_pushed = false;
        // If the previous instruction was an RAII ctor, and we're still in the
        // same BB, then this instruction should become the start of a new BB
        if (prev_inst_was_raii_start) {
          split_points.push_back(&*ii);
          prev_inst_was_raii_start = false;
          already_pushed = true;
        }
        // Check if current instruction is an RAII ctor or dtor
        CallSite CS(cast<Value>(&*ii));
        if (CS) {
          if (Function *Callee = CS.getCalledFunction()) {
            if (Callee->getName() == TM_RAII_CTOR) {
              // Split, so that this ctor is the first instruction in its BB
              if (!already_pushed) {
                split_points.push_back(&*ii);
              }
              // Prepare to split after the ctor if it's not the last instr
              prev_inst_was_raii_start = true;
              // Save the ctor
              raii_region_t temp;
              temp.ctor_inst = &*ii;
              ctors.push_back(temp);
            }
            if (Callee->getName() == TM_RAII_DTOR) {
              // Split, so that this dtor is the first instruction in its BB
              if (!already_pushed) {
                split_points.push_back(&*ii);
              }
            }
          }
        }
      }
    }
  }

  // Actually do the splitting
  for (auto ii : split_points) {
    BasicBlock *BB_to_split = ii->getParent();
    BB_to_split->splitBasicBlock(ii);
  }

  return ctors;
}

/// Find the last entry in ctors_dtors that corresponds to a ctor
Instruction *get_last_ctor(vector<Instruction *> &ctors_dtors) {
  Instruction *res = nullptr;
  for (auto ii : ctors_dtors) {
    CallSite CS(cast<Value>(&*ii));
    if (CS.getCalledFunction()->getName() == TM_RAII_CTOR) {
      res = ii;
    }
  }
  return res;
}

/// analyze a path of ctor and dtor calls that were found during a dfs.  If the
/// path contains an equal number of ctors and dtors, then pair them correctly
/// and return the pairs as a vector.  Otherwise return an empty vector.
///
/// Here are some examples of good, bad, and impossible cases:
///
/// BAD: ctor; ctor
/// BAD: ctor; ctor; dtor
/// BAD: ctor; ctor; dtor; ctor; dtor
/// IMPOSSIBLE: dtor
/// IMPOSSIBLE: ctor; dtor; dtor
/// GOOD: ctor; dtor
/// GOOD: ctor; ctor; dtor; dtor
/// GOOD: ctor; ctor; dtor; ctor; dtor; dtor
vector<raii_region_t>
find_scope_boundaries(vector<Instruction *> &ctors_dtors) {
  // Run a few quick tests to see if we have an equal number of ctors and dtors
  if (ctors_dtors.size() % 2 != 0) {
    return {}; // if not even, can't possibly have 1:1 dtor to ctor mapping
  }
  int ctors = 0, dtors = 0, errors = 0;
  for (auto ii : ctors_dtors) {
    CallSite CS(cast<Value>(&*ii));
    if (CS.getCalledFunction()->getName() == TM_RAII_CTOR) {
      ++ctors;
    } else if (CS.getCalledFunction()->getName() == TM_RAII_DTOR) {
      ++dtors;
    } else {
      ++errors; // NB: should never happen
    }
  }
  if (errors > 0 || ctors != dtors) {
    return {}; // if ctors != dtors, can't have 1:1 dtor to ctor mapping
  }

  // All good... start making matches from scoped pairs
  vector<raii_region_t> result;
  stack<Instruction *> ctor_stack;
  for (auto ii : ctors_dtors) {
    CallSite CS(cast<Value>(&*ii));
    if (CS.getCalledFunction()->getName() == TM_RAII_CTOR) {
      ctor_stack.push(ii);
    } else {
      Instruction *start = ctor_stack.top();
      ctor_stack.pop();
      result.push_back(raii_region_t(start, ii));
    }
  }
  return result;
}

/// Depth-first search to complete the entries in raii_regions.  Initially, each
/// raii_region entry has a reference to a ctor.  We need to find the matching
/// dtor for that ctor, and we also need to find all the BBs between the ctor
/// and matching dtor.
///
/// dfs() is a recursive function.  To avoid infinite recursion, we need to
/// "tree-ify" the CFG.  We do this by explicitly keeping track of the current
/// traversal path.  Before recursing into a BB, we make sure the BB isn't
/// already in the traversal_path, and if it is, we do not recurse into it.
///
/// @param curr_bb         The starting point for this part of the dfs
/// @param ctors_dtors     A collection of all the ctors and dtors that we've
///                        found in the current search
/// @param traversal_path  An ordered list of the BBs that we have visited as
///                        part of the current recursion chain
/// @param bb_worklist     A reference to the raii_regions field of the plugin
/// @param assigned_blocks A set of all the BBs that the dfs has (1)
///                        encountered, and (2) assigned to an raii_region_t
void dfs(BasicBlock *curr_bb, vector<Instruction *> &ctors_dtors,
         vector<BasicBlock *> &traversal_path,
         vector<raii_region_t> &bb_worklist,
         set<BasicBlock *> &assigned_blocks) {
  // Add the current BB to the traversal path
  traversal_path.push_back(curr_bb);

  // If the BB has a ctor or dtor, we need to put the corresponding instruction
  // into the ctors_dtors list.  If the ctor is Invoked, then we also need to
  // track its unwind destination BB.
  //
  // NB: It cannot have both a ctor and dtor, since we ran
  //     normalize_raii_boundaries
  Instruction *ctor = nullptr, *dtor = nullptr;
  BasicBlock *unwind_dest = nullptr;
  for (auto &ii : curr_bb->getInstList()) {
    CallSite CS(&ii);
    if (CS)
      if (Function *Callee = CS.getCalledFunction()) {
        if (Callee->getName() == TM_RAII_CTOR) {
          ctor = &ii;
          ctors_dtors.push_back(ctor);
          if (InvokeInst::classof(ctor))
            unwind_dest = cast<InvokeInst>(ctor)->getUnwindDest();
        } else if (Callee->getName() == TM_RAII_DTOR) {
          dtor = &ii;
          ctors_dtors.push_back(dtor);
        }
      }
  }

  // If curr_bb has a dtor, we may be able to terminate this traversal.  The
  // challenge is that the dtor needs to be ending a top-level transaction, not
  // a nested one.  ctors_dtors has the call stack of raii constructors and
  // destructors on the current traversal_path.  We know we started with a ctor,
  // and we assume that ctors and dtors have a proper lexical scope, so what we
  // need is for there to be an equal number of ctors and dtors.
  // find_scope_boundaries() will do the check, and either return an empty
  // vector, or a vector of (possibly nested) scope boundaries for the current
  // traversal path.  On a non-empty return, we merge the new scope pairs into
  // bb_worklist and move on to the next traversal_path.
  //
  // WARNING: subsequent code in this function relies on a specific behavior of
  //          find_scope_boundaries, namely that when transaction X is nested in
  //          transaction Y, then X will appear earlier in the result vector
  //          than Y.  If this were not true, then X's instructions would get
  //          assigned to Y in the /for (auto bb : traversal_path)/ loop below.
  if (dtor) {
    vector<raii_region_t> pairs = find_scope_boundaries(ctors_dtors);
    if (!pairs.empty()) {
      for (auto pair : pairs) {
        // if this pair is already in bb_worklist, then we may need to set the
        // worklist entry's dtor.  If it's not in bb_worklist, then we need to
        // add it.
        //
        // NB: this code assumes that when a ctor has two dtors, the second one
        //     is a landing pad and isn't really all that important to track
        raii_region_t *region = nullptr;
        for (auto &entry : bb_worklist) {
          // handle a pair whose ctor we've already seen
          if (entry.ctor_inst == pair.ctor_inst) {
            // First dtor is assumed to be the "main" one
            if (entry.dtor_inst == nullptr) {
              entry.dtor_inst = pair.dtor_inst;
            }
            // Subsequent dtors, if any, are landing pads.  We don't need to
            // record these new terminators for a scope, but we do need to
            // consider these different paths...
            region = &entry;
            break;
          }
        }
        // If we didn't find the entry already, we need to add it to bb_worklist
        if (region == nullptr) {
          bb_worklist.push_back(raii_region_t(pair.ctor_inst, pair.dtor_inst));
          region = &bb_worklist.back();
        }

        // we know that traversal_path begins with a ctor and ends with a block
        // whose first instruction is a dtor.  All the remaining basic blocks
        // need to be assigned to the instruction_blocks collection of the
        // appropriate entry in bb_worklist.  Note that each block should only
        // be assigned to *one* bb_worklist entry.
        //
        // WARNING: see above: we are relying on a specific and subtle ordering
        //          of entries in pairs.
        bool record = false;
        // WARNING: the following code is inefficient: it iterates through
        //          traversal_path multiple times.  This is probably OK, since
        //          we don't expect much nesting.  However, it is not ideal.
        for (auto bb : traversal_path) {
          // If the current BB is the BB where this region's dtor resides, we
          // shouldn't assign any more instructions to this region.
          if (pair.dtor_inst->getParent() == bb) {
            record = false;
          }
          // If recording is on, and this block hasn't already been assigned to
          // any region, then assign it to the current region
          else if (record &&
                   assigned_blocks.find(bb) == assigned_blocks.end()) {
            region->instruction_blocks.push_back(bb);
            assigned_blocks.insert(bb);
          }
          // If the current BB is the BB that holds exactly and only the current
          // region's ctor, then we should start assigning instructions to this
          // region.
          else if (pair.ctor_inst->getParent() == bb) {
            record = true;
          }
        }
      }

      // There is no point in continuing to look at the BBs that follow curr_bb
      // on traversal_path: they can not part of a transactional scope until
      // after we encounter a ctor.  But if we'd run into another ctor, then
      // we'd also get to it via some entry in raii_regions, which gets handled
      // via the /for/ loop in discover_raii.
      //
      // However, there may be other paths with the same root as traversal_path,
      // so we can't back all the way out of the dfs... we need to just back out
      // one instruction (i.e., one dtor) and then let the dfs keep going.
      traversal_path.pop_back();
      ctors_dtors.pop_back();
      return;
    }
  }

  // traverse the successors of curr_bb
  for (auto BB : successors(curr_bb)) {
    // There are two tricky parts to this code.  The first is that we need a way
    // to avoid infinite loops.  If a BB's successor is a BB that we've seen
    // already, it means we have a back-edge in the CFG.  In that case, we skip
    // the BB and recurse into its successors.
    if (find(traversal_path.begin(), traversal_path.end(), BB) !=
        traversal_path.end()) {
      for (auto bb : successors(BB)) {
        if (find(traversal_path.begin(), traversal_path.end(), bb) ==
            traversal_path.end()) {
          dfs(bb, ctors_dtors, traversal_path, bb_worklist, assigned_blocks);
        }
      }
    }

    // The second tricky thing is that we may be dealing with a BB that is an
    // unwind destination for some ctor that was invoked.  If it is, then the
    // code within it does *not* belong to the current ctor, but to the ctor
    // that precedes it.  In this case we pop the ctor, recurse, then push it
    // back, but only if this is a nested transaction.
    else if (unwind_dest && BB == unwind_dest && ctors_dtors.size() > 1) {
      // Get the last ctor to be added to ctors_dtors
      Instruction *last_ctor = get_last_ctor(ctors_dtors);
      // Instruction *last_ctor = ctors_dtors[ctors_dtors->size() - 1];
      ctors_dtors.pop_back();
      dfs(BB, ctors_dtors, traversal_path, bb_worklist, assigned_blocks);
      ctors_dtors.push_back(last_ctor);
      continue;
    }

    // In the normal case, we can just recurse
    else {
      dfs(BB, ctors_dtors, traversal_path, bb_worklist, assigned_blocks);
    }
  }

  // When we've run out of BBs to add to the path, we pop back one level and let
  // the DFS keep running
  traversal_path.pop_back();
  if (ctor || dtor)
    ctors_dtors.pop_back();
}

} // namespace

/// Search through the module and find every RAII region.  An RAII region is
/// defined by a matched pair of ctor and dtor calls.  Note that there is
/// transaction nesting, so we can't just start at a ctor and stop at the first
/// dtor we find.
///
/// In addition to *finding* these pairs, we do some slight manipulation, so
/// that each ctor or dtor call is in its own basic block.  The end state of the
/// call is that the plugin's raii_regions collection is properly populated.
void tm_plugin::discover_raii(Module &M) {
  // Find the RAII start points, and make sure each ctor/dtor is in its own BB
  //
  // NB: entries in raii_regions will only have the ctor field set, not dtor or
  //     instruction_blocks
  raii_regions = normalize_raii_boundaries(M);

  // Find the end point for each of the start points we just found, and also
  // find the set of blocks between that start point and end point.
  for (auto &cfg : raii_regions) {
    // Unfortunately, we can't just use the LLVM dominator and postdominator
    // functions, because LLVM InvokeInst and landing pads don't actually result
    // in instructions in RAII regions having common dominators and
    // postdominators in all cases.  Instead, we have to run a manual
    // depth-first search.

    // The dfs may recursively end up working on another cfg in raii_regions.
    // The way we know if a region is done is by seeing if we've got a dtor
    // paired with the ctor
    if (cfg.dtor_inst == NULL) {
      // The dfs needs to track all the ctors and dtors it encounters
      vector<Instruction *> ctors_dtors;

      // The dfs also needs to track every BB it has assigned to a transaction
      set<llvm::BasicBlock *> assigned_blocks;

      // Lastly, the dfs needs to be able to search along the path it has
      // traversed through the CFG, so we need a way to store that path
      vector<BasicBlock *> path;

      dfs(cfg.ctor_inst->getParent(), ctors_dtors, path, raii_regions,
          assigned_blocks);
    }
  }
}

/// For each sub-CFG of the program that is bounded by an RAII ctor and dtor,
/// search through all of its basic blocks and find any function calls/invokes.
/// For each, add the function to func_worklist, but only if we have the
/// definition for the function.
void tm_plugin::discover_reachable_funcs_raii(Module &M) {
  for (auto &cfg : raii_regions) {
    for (auto bb : cfg.instruction_blocks) {
      for (auto &inst : *bb) {
        CallSite CS(&inst);
        if (CS) {
          if (Function *Callee = CS.getCalledFunction()) {
            if (!(Callee->isDeclaration())) {
              func_worklist.push(Callee);
            }
          }
        }
      }
    }
  }
}

// Go through each RAII region, and for each instruction within the region,
// instrument it.  Our current strategy for the RAII API is to replace each
// instruction with a "diamond": a branch that looks at whether the TM system
// has dynamically decided that instrumentation is needed or not, a path that
// does the instrumentation, and a path that doesn't.  We expect LLVM to be
// smart enough to combine these branches, since they are based on a value that
// is immutable for the duration of the transaction.
void tm_plugin::instrument_raii_regions(llvm::Module &M) {
  // In order to make a "diamond" out of an instruction, we need to have the
  // Value* representing the bool that the current transaction uses to determine
  // whether we take the instrumented path or not.  We associate each
  // to-be-instrumented instruction with the appropriate Value* via this map.
  unordered_map<llvm::Instruction *, Value *> inst_pred_map;

  // For the time being, we put every instruction-to-transform into this set. We
  // probably don't need it in the long run.
  set<llvm::Instruction *> instrs_to_instrument;

  // populate inst_pred_map and instrs_to_instrument
  for (auto &cfg : raii_regions)
    for (auto bb : cfg.instruction_blocks) {
      // Get the predicate value
      Value *pred_val = CallSite(cfg.ctor_inst).getArgument(1);

      // Find the instructions to instrument.  They are memory accesses (load,
      // store, AtomicRMW, AtomicCmpXchg) and non-pure function calls/invokes.
      for (auto inst = bb->begin(), E = bb->end(); inst != E; inst++) {
        // is it a memory access?
        if (isa<AtomicRMWInst>(inst) || isa<AtomicCmpXchgInst>(inst) ||
            isa<StoreInst>(inst) || isa<LoadInst>(inst)) {
          instrs_to_instrument.insert(&*inst);
          inst_pred_map[&*inst] = pred_val;
        }
        // is it a non-pure function call/invoke?
        else {
          CallSite CS(cast<Value>(inst));
          if (CS) {
            // skip if it is in the purelist
            if (find(purelist, CS.getCalledFunction()) == purelist.end()) {
              instrs_to_instrument.insert(&*inst);
              inst_pred_map[&*inst] = pred_val;
            }
          }
        }
      }
    }

  // Now that we've got all the instructions that need to be instrumented, let's
  // instrument each one
  for (auto inst : instrs_to_instrument) {
    // The first step is to replace the instruction with a generic pattern of
    // basic blocks
    //
    // Input: a basic block (bb_orig) containing some (possibly empty) prefix
    //        set of instructions p, an instruction ii that needs to be split,
    //        and then some (possibly empty) suffix set of instructions s:
    //
    //   +--bb_orig--+
    //   | {Instr} p |
    //   | Instr ii  |
    //   | {Instr} s |
    //   +-----------+
    //
    // Output: bb_orig changes, and three new blocks are inserted:
    //
    //   +-----bb_orig-----+     +----bb_inst----+
    //   | {Instr} p       |  /->| Instr ii      |
    //   | if (take inst)  | /   | jump bb_done  |---+
    //   | jump BB_inst    |/    +---------------+   |
    //   | else            |                         |  +--bb_done--+
    //   | jump BB_noinst  |---->+---bb_noinst---+   |->| Instr ii  |
    //   +-----------------+     | Instr ii      |   |  | {Instr} s |
    //                           | jump bb_done  |---+  +-----------+
    //                           +---------------+
    //
    // NB: We expect the compiler to be able to do a lot of optimization of
    //     these branches
    IRBuilder<> builder(inst);

    BasicBlock *bb_orig = inst->getParent();
    BasicBlock *bb_done = bb_orig->splitBasicBlock(inst);
    auto ii_done = &*bb_done->begin();

    BasicBlock *bb_inst = BasicBlock::Create(
        M.getContext(), "if_true", inst->getParent()->getParent(), bb_done);
    auto ii_inst = ii_done->clone();
    bb_inst->getInstList().push_back(ii_inst);

    BasicBlock *bb_noinst = BasicBlock::Create(
        M.getContext(), "if_false", inst->getParent()->getParent(), bb_done);
    auto ii_noinst = ii_done->clone();
    bb_noinst->getInstList().push_back(ii_noinst);

    builder.SetInsertPoint(bb_inst);
    builder.CreateBr(bb_done);
    builder.SetInsertPoint(bb_noinst);
    builder.CreateBr(bb_done);
    builder.SetInsertPoint(bb_orig);
    bb_orig->getTerminator()->eraseFromParent();

    Value *support = builder.CreateLoad(inst_pred_map[inst], "support");
    auto do_inst = builder.CreateICmpEQ(support, (Value *)builder.getInt8(1));
    builder.CreateCondBr(do_inst, bb_inst, bb_noinst);

    // At this point, we have the right structure.  The next step is to
    // figure out if bb_noinst needs to be empty and/or bb_done needs a phi node
    //
    // NB: transformation of bb_inst.ii happens later

    // For callsites, the function may have a return value:
    // --> yes: replace bb_done.ii with a phi node
    // --> no:  remove ii from bb_done
    //
    // NB: we need to do this slightly differently for InvokeInst vs. CallInst
    CallSite callsite(cast<Value>(inst));
    if (callsite) {
      // InvokeInst
      if (InvokeInst::classof(inst)) {
        bb_inst->getTerminator()->eraseFromParent();
        bb_noinst->getTerminator()->eraseFromParent();
        if (!dyn_cast<Value>(inst)->getType()->isVoidTy()) {
          // non-void return type: replace bb_done.ii with a phi node
          auto dest = dyn_cast<InvokeInst>(ii_done)->getSuccessor(0);
          builder.SetInsertPoint(&*dest->begin());
          auto phi = builder.CreatePHI(dyn_cast<Value>(ii_done)->getType(), 2);
          phi->addIncoming(dyn_cast<Value>(ii_noinst), bb_noinst);
          phi->addIncoming(dyn_cast<Value>(ii_inst), bb_inst);
          ii_done->moveBefore(phi);
          phi->removeFromParent();
          ReplaceInstWithInst(ii_done, phi);
          // Since this is an InvokeInst, bb_done is empty... remove it
          bb_done->eraseFromParent();
        } else {
          // void return type: we should just remove ii_done
          //
          // NB: Since this is an InvokeInst, ii_done is the only instruction in
          //     bb_done, so we can just remove bb_done
          bb_done->eraseFromParent();
        }
      }
      // CallInst
      else {
        if (!dyn_cast<Value>(inst)->getType()->isVoidTy()) {
          // non-void return type: replace bb_done.ii with a phi node
          builder.SetInsertPoint(&*bb_done->begin());
          auto phi = builder.CreatePHI(dyn_cast<Value>(ii_done)->getType(), 2);
          phi->addIncoming(dyn_cast<Value>(ii_noinst), bb_noinst);
          phi->addIncoming(dyn_cast<Value>(ii_inst), bb_inst);
          phi->removeFromParent();
          ReplaceInstWithInst(ii_done, phi);
        } else {
          // Void return type, so just remove bb_done.ii
          ii_done->eraseFromParent();
        }
      }
    }

    // For atomic instructions, we should make bb_noinst empty.  Later,
    // bb_inst.ii will be replaced with a TM_UNSAFE call, which will then
    // precede the call to bb_done.ii
    else if (isa<AtomicRMWInst>(inst) || isa<AtomicCmpXchgInst>(inst)) {
      ii_noinst->eraseFromParent();
    }

    // For store instructions, we should remove bb_done.ii.  Later, bb_inst.ii
    // will be replaced with a TM_STORE, but we won't need to worry about a phi
    // node.
    else if (isa<StoreInst>(inst)) {
      ii_done->eraseFromParent();
    }

    // For load instructions, we need to replace bb_done.ii with a phi node.
    // Later, bb_inst.ii will be replaced with a TM_LOAD
    else if (isa<LoadInst>(inst)) {
      builder.SetInsertPoint(&*bb_done->begin());
      auto phi = builder.CreatePHI(dyn_cast<Value>(ii_done)->getType(), 2);
      phi->addIncoming(dyn_cast<Value>(ii_noinst), bb_noinst);
      phi->addIncoming(dyn_cast<Value>(ii_inst), bb_inst);
      phi->removeFromParent();
      ReplaceInstWithInst(ii_done, phi);
    }

    // Now we can do the instrumentation of bb_inst.ii
    instrument_raii_instruction(ii_inst->getIterator(), bb_inst->getIterator());
  }
}

/// Transform the clones of an instruction (in bb_inst) that are within a
/// "diamond"
void tm_plugin::instrument_raii_instruction(BasicBlock::iterator inst,
                                            Function::iterator bb) {
  CallSite callsite(cast<Value>(inst));
  if (callsite) {
    // If transform_callsite returns an instruction, then we should
    // use that instruction instead of the one we had
    if (Instruction *new_inst = transform_callsite(callsite, inst)) {
      ReplaceInstWithValue(bb->getInstList(), inst, new_inst);
    }
  }

  // If this is an atomic/volatile RMW memory access, or a fence, we replace the
  // instruction with TM_UNSAFE
  else if (isa<AtomicRMWInst>(inst) || isa<AtomicCmpXchgInst>(inst)) {
    prefix_with_unsafe(inst);
    inst->eraseFromParent();
  }

  // If this is a store instruction, either convert it to a function call or
  // treat it as unsupported (conversion returns nullptr for atomic/volatile
  // stores)
  else if (isa<StoreInst>(inst)) {
    StoreInst *store = dyn_cast<StoreInst>(&*inst);
    if (CallInst *new_store = convert_store(store)) {
      ReplaceInstWithInst(store, new_store);
    } else {
      prefix_with_unsafe(inst);
    }
  }

  // If this is a load instruction, either convert it to a function call or
  // treat it as unsupported (conversion returns nullptr for atomic/volatile
  // loads)
  else if (isa<LoadInst>(inst)) {
    LoadInst *load = dyn_cast<LoadInst>(&*inst);
    // The normal behavior is to use convert_load to instrument this load.  If
    // convert_load returns nullptr, then we need to prefix with unsafe, because
    // it's a volatile or atomic, which is not supported.
    if (instrument_reads) {
      if (Instruction *new_load = convert_load(load)) {
        ReplaceInstWithInst(load, new_load);
      } else {
        prefix_with_unsafe(inst);
      }
    }
    // We don't expect to have volatile or atomic accesses within transactions,
    // but just to be safe, we're not going to just say "hey, it's a load, don't
    // instrument".  Instead, we will say "if it's a /regular/ load, then don't
    // instrument.  If it's volatile or atomic, then do as we would with a
    // volatile or atomic store."
    else {
      if (load->isVolatile() || load->isAtomic()) {
        prefix_with_unsafe(inst);
      }
    }
  }
}