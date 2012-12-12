#ifndef __IDENTIFY_BACK_EDGES_H
#define __IDENTIFY_BACK_EDGES_H

#include <vector>

#include "llvm/Function.h"
#include "llvm/Pass.h"

#include "rcs/typedefs.h"

using namespace llvm;

namespace rcs {
struct IdentifyBackEdges: public FunctionPass {
  enum Color {
    WHITE = 0,
    GRAY,
    BLACK
  };

  static char ID;

  IdentifyBackEdges(): FunctionPass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &O, const Module *M) const;
  const std::vector<BBPair> &getBackEdges() const { return BackEdges; }

 private:
  void DFS(BasicBlock *X);

  std::vector<BBPair> BackEdges;
  DenseMap<BasicBlock *, Color> BBColor;
};
}

#endif
