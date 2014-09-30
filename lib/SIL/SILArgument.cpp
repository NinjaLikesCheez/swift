//===--- SILArgument.cpp - Arguments for high-level SIL code ---------------==//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/STLExtras.h"
#include "swift/SIL/SILBasicBlock.h"
#include "swift/SIL/SILArgument.h"
#include "swift/SIL/SILFunction.h"
#include "swift/SIL/SILInstruction.h"
#include "swift/SIL/SILModule.h"

using namespace swift;

//===----------------------------------------------------------------------===//
// SILArgument Implementation
//===----------------------------------------------------------------------===//

SILArgument::SILArgument(SILType Ty, SILBasicBlock *ParentBB,
                         const ValueDecl *D)
  : ValueBase(ValueKind::SILArgument, Ty), ParentBB(ParentBB), Decl(D) {
  // Function arguments need to have a decl.
  assert(
    !ParentBB->getParent()->isBare() &&
    ParentBB->getParent()->size() == 1
          ? D != nullptr
          : true );
  ParentBB->addArgument(this);
}


SILFunction *SILArgument::getFunction() {
  return getParent()->getParent();
}
const SILFunction *SILArgument::getFunction() const {
  return getParent()->getParent();
}

SILModule &SILArgument::getModule() const {
  return getFunction()->getModule();
}

bool SILArgument::getIncomingValues(llvm::SmallVectorImpl<SILValue> &OutArray) {
  SILBasicBlock *Parent = getParent();

  if (Parent->pred_empty())
    return false;

  unsigned Index = getIndex();
  for (SILBasicBlock *Pred : getParent()->getPreds()) {
    TermInst *TI = Pred->getTerminator();

    if (auto *BI = dyn_cast<BranchInst>(TI)) {
      OutArray.push_back(BI->getArg(Index));
      continue;
    }

    if (auto *CBI = dyn_cast<CondBranchInst>(TI)) {
      OutArray.push_back(CBI->getArgForDestBB(getParent(), this));
      continue;
    }

    if (auto *CCBI = dyn_cast<CheckedCastBranchInst>(TI)) {
      OutArray.push_back(CCBI->getOperand());
      continue;
    }

    if (auto *SWEI = dyn_cast<SwitchEnumInst>(TI)) {
      OutArray.push_back(SWEI->getOperand());
      continue;
    }
    
    return false;
  }

  return true;
}

bool SILArgument::isSelf() const {
  // First make sure that we are actually a function argument. We use an assert
  // boolean return here since in release builds we want to conservatively
  // return false and in debug builds assert since this is a logic error.
  bool isArg = isFunctionArg();
  assert(isArg && "Only function arguments can be self");
  if (!isArg)
    return false;

  // Return true if we are the last argument of our BB and that our parent
  // function has a call signature with self.
  return getFunction()->hasSelfArgument() && getParent()->getBBArgs().back();
}
