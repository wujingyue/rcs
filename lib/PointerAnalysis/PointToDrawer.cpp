#include <cstdio>
#include <set>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "rcs/IDAssigner.h"
#include "rcs/PointerAnalysis.h"
using namespace rcs;

namespace rcs {
struct PointToDrawer: public ModulePass {
  static char ID;

  PointToDrawer(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

 private:
  // Print the point-to graph as a .dot file. 
  void printToDot(raw_ostream &O);
};
}

static cl::opt<string> DotFileName("pointer-dot",
                                   cl::desc("The output graph file name"));
static cl::opt<bool> ShouldPrintStat("pointer-stats",
                                     cl::desc("Print stat info of "
                                              "PointerAnalysis"));

char PointToDrawer::ID = 0;

void PointToDrawer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<PointerAnalysis>();
}

void PointToDrawer::printToDot(raw_ostream &O) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();
  PointerAnalysis &PA = getAnalysis<PointerAnalysis>();

  O << "strict digraph PointTo {\n";

  set<unsigned> PointerVids, PointeeVids;
  ValueList Pointers;
  PA.getAllPointers(Pointers);
  for (size_t i = 0; i < Pointers.size(); ++i) {
    Value *Pointer = Pointers[i];
    assert(Pointer != NULL);
    assert(Pointer->getType()->isPointerTy());

    unsigned PointerVid = IDA.getValueID(Pointer);
    assert(PointerVid != IDAssigner::InvalidID);
    PointerVids.insert(PointerVid);

    ValueList Pointees;
    PA.getPointees(Pointer, Pointees);

    for (size_t j = 0; j < Pointees.size(); ++j) {
      Value *Pointee = Pointees[j];
      assert(Pointee != NULL);

      unsigned PointeeVid = IDA.getValueID(Pointee);
      assert(PointeeVid != IDAssigner::InvalidID);
      PointeeVids.insert(PointeeVid);
      
      O << "TopLevel" << PointerVid << " -> AddrTaken" << PointeeVid << "\n";
    }
  }
  
  for (set<unsigned>::iterator I = PointerVids.begin(); I != PointerVids.end();
       ++I) {
    O << "TopLevel" << *I << " ";
    O << "[label = " << *I << "]\n";
  }

  for (set<unsigned>::iterator I = PointeeVids.begin(); I != PointeeVids.end();
       ++I) {
    O << "AddrTaken" << *I << " ";
    O << "[label = " << *I << ", ";
    O << "style = filled, ";
    O << "fillcolor = yellow]\n";
  }

  O << "}\n";
}

bool PointToDrawer::runOnModule(Module &M) {
  PointerAnalysis &PA = getAnalysis<PointerAnalysis>();

  if (DotFileName != "") {
    string ErrorInfo;
    raw_fd_ostream DotFile(DotFileName.c_str(), ErrorInfo);
    printToDot(DotFile);
  }

  if (ShouldPrintStat) {
    PA.printStats(errs());
  }

  return false;
}

static RegisterPass<PointToDrawer> X("draw-point-to",
                                     "Draw point-to graphs",
                                     false, // Is CFG Only? 
                                     true); // Is Analysis? 
