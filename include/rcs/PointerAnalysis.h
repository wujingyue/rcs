#ifndef __RCS_POINTER_ANALYSIS_H
#define __RCS_POINTER_ANALYSIS_H

#include "llvm/Value.h"

#include "rcs/typedefs.h"
#include "rcs/IDAssigner.h"

namespace rcs {
struct PointerAnalysis {
  // A must for an AnalysisGroup.
  static char ID;

  // We want to be subclassed. 
  virtual ~PointerAnalysis() {}
  // Returns true if we have point-to information for <Pointer>. 
  // Due to some limitations of underlying alias analyses, it is not always
  // possible to capture all pointers. 
  virtual bool getPointees(const llvm::Value *Pointer,
                           rcs::ValueList &Pointees) = 0;
  virtual void getAllPointers(rcs::ValueList &Pointers) = 0;
  // Print some stat information to <O>. 
  void printStats(raw_ostream &O);

 protected:
  PointerAnalysis() {}
};
}

#endif
