#define DEBUG_TYPE "rcs-cfg"

#include "llvm/ADT/Statistic.h"

#include "rcs/IdentifyBackEdges.h"

using namespace std;
using namespace llvm;
using namespace rcs;

static RegisterPass<IdentifyBackEdges> X("identify-back-edges",
                                         "Identify back edges",
                                         false,
                                         true);

STATISTIC(NumBackEdges, "Number of back edges");

char IdentifyBackEdges::ID = 0;

void IdentifyBackEdges::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

// The standard DFS algorithm. Back edges go from white nodes to gray nodes.
void IdentifyBackEdges::DFS(BasicBlock *X,
                            DenseMap<BasicBlock *, Color> &BBColor) {
  BBColor[X] = GRAY;
  TerminatorInst *TI = X->getTerminator();
  for (unsigned i = 0; i < TI->getNumSuccessors(); ++i) {
    BasicBlock *Y = TI->getSuccessor(i);
    if (BBColor[Y] == GRAY) {
      BackEdges.push_back(BBPair(X, Y));
      continue;
    }
    if (BBColor[Y] == WHITE) {
      DFS(Y, BBColor);
    }
  }
  BBColor[X] = BLACK;
}

bool IdentifyBackEdges::runOnModule(Module &M) {
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (F->isDeclaration())
      continue;
    DenseMap<BasicBlock *, Color> BBColor;
    DFS(F->begin(), BBColor);
  }

  sort(BackEdges.begin(), BackEdges.end());
  BackEdges.erase(unique(BackEdges.begin(), BackEdges.end()), BackEdges.end());
  NumBackEdges = BackEdges.size();

  return false;
}

unsigned IdentifyBackEdges::getID(BasicBlock *B1, BasicBlock *B2) const {
  BBPair P(B1, B2);
  vector<BBPair>::const_iterator I = lower_bound(BackEdges.begin(),
                                                 BackEdges.end(),
                                                 P);
  if (I == BackEdges.end() || *I != P)
    return -1;
  return I - BackEdges.begin();
}
