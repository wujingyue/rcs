#include <string>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace rcs {
struct CallGraphDrawer: public ModulePass {
  static char ID;

  CallGraphDrawer(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

 private:
  void printStats(raw_ostream &O, Module &M);
  void printToDot(raw_ostream &O, Module &M);
  void printCallEdgesFrom(raw_ostream &O, CallGraphNode *CallerNode);
  void printCallEdge(raw_ostream &O, Function *Caller, Function *Callee);
};
}
using namespace rcs;

static RegisterPass<CallGraphDrawer> X("draw-cg",
                                       "Draw the call graph",
                                       false, // Is CFG Only? 
                                       true); // Is Analysis? 

static cl::opt<string> DotFileName("cg-dot",
                                   cl::desc("The output graph file name"));
static cl::opt<bool> ShouldPrintStats("cg-stats",
                                      cl::desc("Print stat info of CallGraph"));

char CallGraphDrawer::ID = 0;

void CallGraphDrawer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<CallGraph>();
}

bool CallGraphDrawer::runOnModule(Module &M) {
  if (DotFileName != "") {
    string ErrorInfo;
    raw_fd_ostream DotFile(DotFileName.c_str(), ErrorInfo);
    printToDot(DotFile, M);
  }

  if (ShouldPrintStats) {
    printStats(errs(), M);
  }

  return false;
}

void CallGraphDrawer::printCallEdgesFrom(raw_ostream &O,
                                         CallGraphNode *CallerNode) {
  assert(CallerNode);
  for (unsigned i = 0; i < CallerNode->size(); ++i) {
    CallGraphNode *CalleeNode = (*CallerNode)[i];
    printCallEdge(O, CallerNode->getFunction(), CalleeNode->getFunction());
  }
}

void CallGraphDrawer::printCallEdge(raw_ostream &O,
                                    Function *Caller, Function *Callee) {
  assert((Caller || Callee) && "Both caller and callee are null");
  O << (Caller ? "\"F_" + Caller->getName() + "\"": "ExternCallingNode");
  O << " -> ";
  O << (Callee ? "\"F_" + Callee->getName() + "\"": "CallsExternNode");
  O << "\n";
}

void CallGraphDrawer::printToDot(raw_ostream &O, Module &M) {
  CallGraph &CG = getAnalysis<CallGraph>();

  O << "strict digraph CallGraph {\n";

  // CallsExternNode and ExternCallingNode
  O << "CallsExternNode [label = bottom]\n";
  O << "ExternCallingNode [label = top]\n";
  // One node for each function. 
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    O << "\"F_" << F->getName() << "\" ";
    O << "[label = \"" << F->getName() << "\"]\n";
  }

  // What does ExternCallingNode call? 
  printCallEdgesFrom(O, CG.getExternalCallingNode());
  // What does each function call? 
  for (Module::iterator F = M.begin(); F != M.end(); ++F)
    printCallEdgesFrom(O, CG[F]);
  
  O << "}\n";
}

void CallGraphDrawer::printStats(raw_ostream &O, Module &M) {
  typedef multimap<unsigned, Function *, greater<unsigned> > Distribution;

  CallGraph &CG = getAnalysis<CallGraph>();

  // We ignore edges coming from or going to the two external nodes. 
  unsigned NumFunctions = 0, NumCallEdges = 0;
  Distribution InDegreeDist, OutDegreeDist;
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    CallGraphNode *Node = CG[F];
    assert(Node);
    ++NumFunctions;
    NumCallEdges += Node->size();
    InDegreeDist.insert(make_pair(Node->getNumReferences(), F));
    OutDegreeDist.insert(make_pair(Node->size(), F));
  }

  // Note that there might be multiple edges between two different
  // functions, because we count call edges for each call site. 
  O << "# of functions = " << NumFunctions << "\n";
  O << "# of call edges = " << NumCallEdges << "\n";

  O << "Top ten in degrees:\n";
  unsigned Limit = 10;
  for (Distribution::iterator I = InDegreeDist.begin();
       I != InDegreeDist.end() && Limit > 0; ++I, --Limit) {
    O << "  " << I->second->getName() << ": " << I->first << "\n";
  }

  O << "Top ten out degrees:\n";
  Limit = 10;
  for (Distribution::iterator I = OutDegreeDist.begin();
       I != OutDegreeDist.end() && Limit > 0; ++I, --Limit) {
    O << "  " << I->second->getName() << ": " << I->first << "\n";
  }
}
