// Author: Jingyue

#include <cstdio>
#include <set>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "common/IDAssigner.h"
#include "common/PointerAnalysis.h"
using namespace rcs;

namespace rcs {
struct PointToDrawer: public ModulePass {
  static char ID;

  PointToDrawer(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &O, const Module *M) const;
};
}

static cl::opt<string> DotFileName("dot",
                                   cl::desc("The output graph file name"
                                            " (.dot)"),
                                   cl::ValueRequired);

char PointToDrawer::ID = 0;

void PointToDrawer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<PointerAnalysis>();
}

bool PointToDrawer::runOnModule(Module &M) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();
  PointerAnalysis &PA = getAnalysis<PointerAnalysis>();

  assert(DotFileName != "");
  string ErrorInfo;
  raw_fd_ostream DotFile(DotFileName.c_str(), ErrorInfo);
  PA.printDot(DotFile, IDA);

  return false;
}

void PointToDrawer::print(raw_ostream &O, const Module *M) const {
  // Do nothing. 
}

static RegisterPass<PointToDrawer> X("draw-point-to",
                                     "Draw point-to graphs",
                                     false, // Is CFG Only? 
                                     true); // Is Analysis? 
