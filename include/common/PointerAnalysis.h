// Author: Jingyue

#ifndef __RCS_POINTER_ANALYSIS_H
#define __RCS_POINTER_ANALYSIS_H

#include "llvm/Value.h"

#include "common/typedefs.h"

namespace rcs {
struct PointerAnalysis {
  // A must for an AnalysisGroup.
  static char ID;

  PointerAnalysis() {}
  // We want to be subclassed. 
  virtual ~PointerAnalysis() {}
  virtual void getPointees(const llvm::Value *Pointer,
                           rcs::ValueList &Pointees) const = 0;
};
}

#endif
