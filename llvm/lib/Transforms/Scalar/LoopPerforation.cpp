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
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PriorityWorklist.h"
#include "llvm/ADT/SmallPtrSet.h"
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
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopPerforation.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
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

// Returns the loop hint metadata node with the given name (for example,
// "llvm.loop.unroll.count").  If no such metadata node exists, then nullptr is
// returned.
static MDNode *getMetadataForLoop(const Loop *L, StringRef Name) {
  if (MDNode *LoopID = L->getLoopID())
    return GetUnrollMetadata(LoopID, Name);

  return nullptr;
}

// Returns true if the loop has an unroll(enable) pragma.
static bool hasPerforationEnablePragma(const Loop *L) {
  return getMetadataForLoop(L, "llvm.loop.perforation.enable");
}

static bool isLoopPerforable(Loop &L, ScalarEvolution &SE) {
  if (!hasPerforationEnablePragma(&L)) {
    errs() << "[FAIL] Pragma\n";
    return false;
  }

  if (!L.isLoopSimplifyForm()) {
    errs() << "[FAIL] Simplify Form\n";
    return false;
  }

  PHINode *PHI = L.getCanonicalInductionVariable();
  if (PHI == nullptr) {
    errs() << "[FAIL] PHI\n";
    return false;
  }

  // Find where the induction varialbe is modified by finding a user that is
  // also a incoming value to the phi
  Value *ValueToChange = nullptr;

  for (auto *User : PHI->users()) {
    for (auto &Incoming : PHI->incoming_values()) {
      if (Incoming == User) {
        ValueToChange = Incoming;
        break;
      }
    }
  }

  if (ValueToChange == nullptr) {
    errs() << "[FAIL] Value to Change\n";
    return false;
  }

  if (!isa<BinaryOperator>(ValueToChange)) {
    errs() << "[FAIL] Binary Operator\n";
    return false;
  }

  return true;
}

PreservedAnalyses LoopPerforationPass::run(Loop &L, LoopAnalysisManager &AM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &U) {
  if (!isLoopPerforable(L, AR.SE)) {
    errs() << "Not Perforated\n";
    return PreservedAnalyses::all();
  }

  // Find the canonical induction variable for this loop
  PHINode *PHI = L.getCanonicalInductionVariable();

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
  for (auto &Op : Increment->operands()) {
    if (Op == PHI)
      continue;

    int LoopRate = 2;
    Type *ConstType = Op->getType();
    Constant *NewInc =
        ConstantInt::get(ConstType, LoopRate /*value*/, true /*issigned*/);

    errs() << "Changing [" << *Op << "] to [" << *NewInc << "]!\n";

    Op = NewInc;
    errs() << "Perforated\n";
    return getLoopPassPreservedAnalyses();
  }

  errs() << "Something went wrong\n";
  return PreservedAnalyses::all();
}
