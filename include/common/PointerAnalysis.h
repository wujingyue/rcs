// Author: Jingyue

#ifndef __RCS_POINTER_ANALYSIS_H
#define __RCS_POINTER_ANALYSIS_H

#include <cstdio>

#include "llvm/Value.h"

#include "common/typedefs.h"

namespace rcs {
struct PointerAnalysis {
  // A must for an AnalysisGroup.
  static char ID;

  // We want to be subclassed. 
  virtual ~PointerAnalysis() {}
  virtual void getPointees(const llvm::Value *Pointer,
                           rcs::ValueList &Pointees) = 0;
 protected:
  PointerAnalysis() {}
};
}

#endif
