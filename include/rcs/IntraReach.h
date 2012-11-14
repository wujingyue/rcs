// DEPRECATED: Use reach instead. It's more general.

#ifndef __INTRA_REACH_H
#define __INTRA_REACH_H

#include "llvm/Pass.h"
using namespace llvm;

#include "rcs/typedefs.h"
using namespace rcs;

/* TODO: Make it work with GraphTraits. Refer to DominatorTree */
namespace rcs {
struct IntraReach: public FunctionPass {
  static char ID;

  IntraReach();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function &F);
  /* Runs in linear time */
  bool reachable(const BasicBlock *x, const BasicBlock *y) const;
  /*
   * Floodfill starting from <x>, ending with <sink>.
   * <visited> will contain all visited BBs.
   */
  void floodfill(
      const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const;
  void floodfill_r(
      const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const;
};
}

#endif
