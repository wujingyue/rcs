// Author: Jingyue

#ifndef __RCS_POINTER_ANALYSIS_H
#define __RCS_POINTER_ANALYSIS_H

#include "llvm/Value.h"
using namespace llvm;

#include "common/typedefs.h"
using namespace rcs;

namespace rcs {
struct PointerAnalysis {
  // A must for an AnalysisGroup.
  static char ID;

  PointerAnalysis() {}
  // We want to be subclassed. 
  virtual ~PointerAnalysis() {}
  virtual void getPointees(const Value *Pointer, ValueSet &Pointees) = 0;
};
}

#endif
