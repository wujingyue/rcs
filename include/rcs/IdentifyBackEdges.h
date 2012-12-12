#ifndef __IDENTIFY_BACK_EDGES_H
#define __IDENTIFY_BACK_EDGES_H

#include <vector>

#include "llvm/Function.h"
#include "llvm/Pass.h"

#include "rcs/typedefs.h"

using namespace llvm;

namespace rcs {
// Make it a ModulePass because we need to assign each back edge an ID.
struct IdentifyBackEdges: public ModulePass {
  enum Color {
    WHITE = 0,
    GRAY,
    BLACK
  };

  static char ID;

  IdentifyBackEdges(): ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  const std::vector<BBPair> &getBackEdges() const { return BackEdges; }
  unsigned getID(BasicBlock *B1, BasicBlock *B2) const;

 private:
  void DFS(BasicBlock *X, DenseMap<BasicBlock *, Color> &BBColor);

  std::vector<BBPair> BackEdges;
};
}

#endif
