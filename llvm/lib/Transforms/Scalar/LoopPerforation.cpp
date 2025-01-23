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
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
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
static bool hasPerforationEnablePragma(const Loop *L) {
  if (!GetUnrollMetadata(L->getLoopID(), "llvm.loop.perforation.enable"))
    return false;

  return true;
}

// Find the canonical induction variable for this loop
static PHINode *getCanonicalVariable(Loop &L) {
  BasicBlock *H = L.getHeader();

  for (auto It = H->begin(); isa<PHINode>(It); It) {
    return cast<PHINode>(It);
  }

  errs() << "[FAIL] PHI\n";
  return nullptr;
}

// Verifies if the loop has all the properties to be perforable
static bool isLoopPerforable(Loop &L, ScalarEvolution &SE) {
  if (!hasPerforationEnablePragma(&L)) {
    errs() << "[FAIL] Pragma\n";
    return false;
  }

  if (!L.isLoopSimplifyForm()) {
    errs() << "[FAIL] Simple Form\n";
    return false;
  }

  PHINode *PN = getCanonicalVariable(L);
  if (!PN) {
    return false;
  }

  return true;
}

PreservedAnalyses LoopPerforationPass::run(Loop &L, LoopAnalysisManager &AM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &U) {
  if (!isLoopPerforable(L, AR.SE)) {
    return PreservedAnalyses::all();
  }

  // Find the canonical induction variable for this loop
  PHINode *PHI = getCanonicalVariable(L);

  // Find where the induction variable is modified by finding a user that
  // is also an incoming value to the phi
  Value *ValueToChange = nullptr;

  for (const auto *User : PHI->users()) {
    for (const auto &Incoming : PHI->incoming_values()) {
      if (Incoming == User) {
        ValueToChange = Incoming;
        break; // TODO: what if there are multiple?
      }
    }
  }

  BinaryOperator *Increment = dyn_cast<BinaryOperator>(ValueToChange);
  if (!Increment) {
    errs() << "[FAIL] Not a binary operator\n";
    return PreservedAnalyses::all();
  }
  
  for (auto &Op : Increment->operands()) {
    if (Op == PHI)
      continue;
    
    ConstantInt *CI = dyn_cast<ConstantInt>(Op);
    if (!CI) {
      errs() << "[FAIL] Not a integer constant\n";
      return PreservedAnalyses::all();
    }

    int64_t LoopRate = CI->getSExtValue();
    if (LoopRate < 0) {
      LoopRate -= 1;
    } else {
      LoopRate += 1;
    }

    Type *ConstType = Op->getType();
    Constant *NewInc =
        ConstantInt::get(ConstType, LoopRate /*value*/, true /*issigned*/);

    Op = NewInc;
    return getLoopPassPreservedAnalyses();
  }

  errs() << "[FAIL] Could not perforate\n";
  return PreservedAnalyses::all();
}
