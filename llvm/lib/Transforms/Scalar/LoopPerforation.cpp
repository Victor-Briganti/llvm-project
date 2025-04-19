//===- LoopPerforation.cpp - Loop perforation pass --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass implements the loop perforation pass.
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/LoopPerforation.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/IVDescriptors.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/RDFGraph.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopPerforation.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

#define DEBUG_TYPE "loop-perforation"

static cl::opt<bool>
    AllowPerforation("allow-perforation", cl::Hidden,
                     cl::desc("Allows loops to be perforated."));

// Returns true if the loop hint metadata node with the given name (for example,
// "llvm.loop.perforation.enable").  If no such metadata node exists, then false
// is returned.
static bool hasPerforationEnablePragma(const Loop &L) {
  if (!GetUnrollMetadata(L.getLoopID(), "llvm.loop.perforation.enable") ||
      !GetUnrollMetadata(L.getLoopID(), "llvm.loop.perforation.count"))
    return false;

  return true;
}

// Changes the induction variable of the Loop
static bool changeInductionVariable(PHINode *PHI, BinaryOperator *Increment,
                                    int64_t IncrementValue) {
  for (auto &Op : Increment->operands()) {
    if (Op == PHI)
      continue;

    ConstantInt *CI = dyn_cast<ConstantInt>(Op);
    if (!CI) {
      // errs() << "[FAIL] Not a integer constant\n";
      return false;
    }

    int64_t LoopRate = CI->getSExtValue();
    if (LoopRate < 0) {
      LoopRate -= IncrementValue;
    } else {
      LoopRate += IncrementValue;
    }

    Type *ConstType = Op->getType();
    Constant *NewInc =
        ConstantInt::get(ConstType, LoopRate /*value*/, true /*issigned*/);

    Op = NewInc;
    return true;
  }

  return false;
}

static bool tryToPerforateLoop(Loop &L, ScalarEvolution &SE) {
  if (!L.isLoopSimplifyForm() || !L.getLoopID()) {
    // errs() << "[FAIL] Loop is not in simplify form\n";
    return false;
  }
  
  int64_t IncrementValue = 1;
  if (MDNode *PerforationCount =
          GetUnrollMetadata(L.getLoopID(), "llvm.loop.perforation.count")) {
    ConstantInt *CI =
        mdconst::extract<ConstantInt>(PerforationCount->getOperand(1));
    if (!CI) {
      // errs() << "[FAIL] Could not extract the int\n";
      return false;
    }

    IncrementValue = CI->getSExtValue();
  } else {
    if (!hasPerforationEnablePragma(L)) {
      // errs() << "[FAIL] Annotation does not exists\n";
      return false;
    }
  }

  for (PHINode &PN : L.getHeader()->phis()) {
    const SCEV *S = SE.getSCEV(&PN);

    // Check to see if the variable is a recurrence
    if (const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(S)) {
      // Ensure that the recurrence belongs to this loop
      if (AR->getLoop() == &L) {
        Value *ValueToChange = nullptr;

        // Verify its incoming values
        for (const auto *User : PN.users()) {
          for (const auto &Incoming : PN.incoming_values()) {
            if (Incoming == User) {
              ValueToChange = Incoming;
              break;
            }
          }
        }

        if (BinaryOperator *Increment =
                dyn_cast<BinaryOperator>(ValueToChange)) {
          return changeInductionVariable(&PN, Increment, IncrementValue);
        }

        // errs() << "[FAIL] Not a binary operator\n";
        return false;
      }
    }
  }

  // errs() << "[FAIL] Could not extract induction variable\n";
  return false;
}

namespace {
struct LoopPerforation : public LoopPass {
public:
  static char ID; // Pass ID, replacement for typeid

  LoopPerforation() : LoopPass(ID) {
    initializeLoopPerforationPass(*PassRegistry::getPassRegistry());
  }

  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    if (skipLoop(L)) {
      return false;
    }

    ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    return tryToPerforateLoop(*L, *SE);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<MemorySSAWrapperPass>();
    AU.addPreserved<MemorySSAWrapperPass>();
    getLoopAnalysisUsage(AU);
  }
};
} // namespace

char LoopPerforation::ID = 0;

INITIALIZE_PASS_BEGIN(LoopPerforation, "loop-perforation",
                      "Perforate canonical loops", false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
INITIALIZE_PASS_END(LoopPerforation, "loop-perforation",
                    "Perforate canonical loops", false, false)

Pass *llvm::createLoopPeforationPass() { return new LoopPerforation(); }

PreservedAnalyses LoopPerforationPass::run(Loop &L, LoopAnalysisManager &AM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &U) {
  if (tryToPerforateLoop(L, AR.SE)) {
    auto PA = getLoopPassPreservedAnalyses();
    PA.preserve<MemorySSAAnalysis>();
    return PA;
  }

  // errs() << "[FAIL] Could not perforate\n";
  return PreservedAnalyses::all();
}
