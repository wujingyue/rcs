// Author: Jingyue

#include <string>
using namespace std;

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Pass.h"
using namespace llvm;

namespace rcs {
struct CallGraphDrawer: public ModulePass {
  static char ID;

  CallGraphDrawer(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
};
}
using namespace rcs;

static RegisterPass<CallGraphDrawer> X("draw-cg",
                                       "Draw the call graph",
                                       false, // Is CFG Only? 
                                       true); // Is Analysis? 

static cl::opt<string> DotFileName("cg-dot",
                                   cl::desc("The output graph file name"));

char CallGraphDrawer::ID = 0;

void CallGraphDrawer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<CallGraph>();
}

bool CallGraphDrawer::runOnModule(Module &M) {
  assert(false);
  return false;
}
