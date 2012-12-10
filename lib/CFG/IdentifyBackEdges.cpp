#include "rcs/IdentifyBackEdges.h"

using namespace rcs;

static RegisterPass<IdentifyBackEdges> X("identify-back-edges",
                                         "Identify back edges",
                                         false,
                                         true);

char IdentifyBackEdges::ID = 0;

void IdentifyBackEdges::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

// The standard DFS algorithm. Back edges go from white nodes to gray nodes.
void IdentifyBackEdges::DFS(BasicBlock *X) {
  BBColor[X] = GRAY;
  TerminatorInst *TI = X->getTerminator();
  for (unsigned i = 0; i < TI->getNumSuccessors(); ++i) {
    BasicBlock *Y = TI->getSuccessor(i);
    if (BBColor[Y] == WHITE) {
      DFS(Y);
    } else if (BBColor[Y] == GRAY) {
      BackEdges.push_back(BBPair(X, Y));
    }
  }
  BBColor[X] = BLACK;
}

bool IdentifyBackEdges::runOnFunction(Function &F) {
  // A FunctionPass is reused for all functions. Do not expect the data
  // structures to be automatically cleared.
  BackEdges.clear();
  BBColor.clear();

  DFS(F.begin());

  return false;
}

void IdentifyBackEdges::print(raw_ostream &O, const Module *M) const {
  Function *F = NULL;
  for (size_t i = 0; i < BackEdges.size(); ++i) {
    BasicBlock *B1 = BackEdges[i].first, *B2 = BackEdges[i].second;
    O << B1->getName() << " -> " << B2->getName() << "\n";
    if (F == NULL)
      F = B1->getParent();
    // All back edges should share the same containing function.
    assert(B1->getParent() == F || B2->getParent() == F);
  }
}
