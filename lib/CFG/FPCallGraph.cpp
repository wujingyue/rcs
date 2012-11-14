// A call-graph builder considering function pointers.
// The targets of function pointers are identified by alias analysis.
// Users may specify which alias analysis she wants to run this pass with.

#include <cstdio>
#include <fstream>

#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/AliasAnalysis.h"

#include "rcs/FPCallGraph.h"
#include "rcs/util.h"

using namespace std;
using namespace llvm;
using namespace rcs;

static RegisterPass<FPCallGraph> X("fpcg",
                         "Call graph that recognizes function pointers",
                         false, true);
static RegisterAnalysisGroup<CallGraph> Y(X);

char FPCallGraph::ID = 0;

void FPCallGraph::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<AliasAnalysis>();
}

void *FPCallGraph::getAdjustedAnalysisPointer(AnalysisID PI) {
  if (PI == &CallGraph::ID)
    return (CallGraph*)this;
  return this;
}

FPCallGraph::FPCallGraph(): ModulePass(ID) {
  Root = NULL;
  ExternCallingNode = NULL;
  CallsExternNode = NULL;
}

void FPCallGraph::destroy() {
  // <CallsExternNode> is not in the function map, delete it explicitly.
  delete CallsExternNode;
  CallsExternNode = NULL;
  CallGraph::destroy();
}

void FPCallGraph::addCallEdge(const CallSite &Site, Function *Callee) {
  Instruction *Ins = Site.getInstruction();
  assert(Ins);
  SiteToFuncs[Ins].push_back(Callee);
  FuncToSites[Callee].push_back(Ins);
  // Update CallGraph as well.
  CallGraphNode *Node = getOrInsertFunction(Ins->getParent()->getParent());
  Node->addCalledFunction(Site, getOrInsertFunction(Callee));
}

template <class T>
void FPCallGraph::MakeUnique(vector<T> &V) {
  sort(V.begin(), V.end());
  V.erase(unique(V.begin(), V.end()), V.end());
}

FuncList FPCallGraph::getCalledFunctions(
    const Instruction *Ins) const {
  SiteToFuncsMapTy::const_iterator I = SiteToFuncs.find(Ins);
  if (I == SiteToFuncs.end())
    return FuncList();
  return I->second;
}

InstList FPCallGraph::getCallSites(
    const Function *F) const {
  FuncToSitesMapTy::const_iterator I = FuncToSites.find(F);
  if (I == FuncToSites.end())
    return InstList();
  return I->second;
}

void FPCallGraph::processCallSite(const CallSite &CS, const FuncSet &AllFuncs) {
  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

  if (Function *Callee = CS.getCalledFunction()) {
    // Ignore calls to intrinsic functions.
    // CallGraph would throw assertion failures.
    if (!Callee->isIntrinsic()) {
      addCallEdge(CS, Callee);
      const Instruction *Ins = CS.getInstruction();
      if (is_pthread_create(Ins)) {
        // Add edge: Ins => the thread function
        Value *Target = get_pthread_create_callee(Ins);
        if (Function *ThrFunc = dyn_cast<Function>(Target)) {
          // pthread_create with a known function
          addCallEdge(CS, ThrFunc);
        } else {
          // Ask AA which functions <target> may point to.
          for (FuncSet::const_iterator I = AllFuncs.begin();
               I != AllFuncs.end(); ++I) {
            if (AA.alias(Target, *I))
              addCallEdge(CS, *I);
          }
        }
      }
    }
  } else {
    Value *FP = CS.getCalledValue();
    assert(FP && "Cannot find the function pointer");
    // Ask AA which functions <fp> may point to.
    for (FuncSet::const_iterator I = AllFuncs.begin();
         I != AllFuncs.end(); ++I) {
      if (AA.alias(FP, *I))
        addCallEdge(CS, *I);
    }
  }
}

bool FPCallGraph::runOnModule(Module &M) {
  // Initialize super class CallGraph.
  CallGraph::initialize(M);

  // Use getOrInsertFunction(NULL) so that
  // ExternCallingNode->getFunction() returns NULL.
  ExternCallingNode = getOrInsertFunction(NULL);
  CallsExternNode = new CallGraphNode(NULL);

  // Every function need to have a corresponding CallGraphNode.
  for (Module::iterator F = M.begin(); F != M.end(); ++F)
    getOrInsertFunction(F);

  /*
   * Get the set of all defined functions.
   * Will be used as a candidate set for point-to analysis.
   * FIXME: Currently we have to skip external functions, otherwise
   * bc2bdd would fail. Don't ask me why.
   */
  FuncSet AllFuncs;
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (!F->isDeclaration())
      AllFuncs.insert(F);
  }

  /* Get Root (main function) */
  unsigned NumMains = 0;
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (!F->hasLocalLinkage() && F->getName() == "main") {
      NumMains++;
      Root = getOrInsertFunction(F);
    }
  }
  // No root if no main function or more than one main functions.
  if (NumMains != 1)
    Root = ExternCallingNode;

  // Connect <ExternCallingNode>
  if (Root != ExternCallingNode) {
    ExternCallingNode->addCalledFunction(CallSite(), Root);
  } else {
    for (Module::iterator F = M.begin(); F != M.end(); ++F) {
      if (!F->hasLocalLinkage()) {
        ExternCallingNode->addCalledFunction(CallSite(),
                                             getOrInsertFunction(F));
      }
    }
  }

  // Connect <CallsExternNode>.
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (F->isDeclaration())
      getOrInsertFunction(F)->addCalledFunction(CallSite(), CallsExternNode);
  }

  // Build the call graph.
  SiteToFuncs.clear();
  FuncToSites.clear();
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator Ins = BB->begin(); Ins != BB->end(); ++Ins) {
        CallSite CS(Ins);
        if (CS.getInstruction())
          processCallSite(CS, AllFuncs);
      }
    }
  }

  // Simplify the call graph.
  simplifyCallGraph();

  return false;
}

void FPCallGraph::simplifyCallGraph() {
  // Remove duplicated items in each vector.
  for (SiteToFuncsMapTy::iterator I = SiteToFuncs.begin();
       I != SiteToFuncs.end(); ++I) {
    MakeUnique(I->second);
  }
  for (FuncToSitesMapTy::iterator I = FuncToSites.begin();
       I != FuncToSites.end(); ++I) {
    MakeUnique(I->second);
  }
}

void FPCallGraph::print(llvm::raw_ostream &O, const Module *M) const {
  O << "Caller - Callee:\n";
  for (Module::const_iterator F = M->begin(); F != M->end(); ++F) {
    // All called functions inside <F>.
    FuncList AllCallees;
    for (Function::const_iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::const_iterator Ins = BB->begin();
           Ins != BB->end(); ++Ins) {
        if (is_call(Ins)) {
          const FuncList &CalledFunctions = getCalledFunctions(Ins);
          for (FuncList::const_iterator I = CalledFunctions.begin();
               I != CalledFunctions.end(); ++I)
            AllCallees.push_back(*I);
        }
      }
    }
    MakeUnique(AllCallees);
    if (!AllCallees.empty()) {
      O << "\t" << F->getName() << " calls:\n";
      forall(FuncList, I, AllCallees)
          O << "\t\t" << (*I)->getName() << "\n";
    }
  }
  O << "Callee - Caller:\n";
  for (Module::const_iterator F = M->begin(); F != M->end(); ++F) {
    // All calling functions to <F>.
    const InstList &CallSites = getCallSites(F);
    FuncList AllCallers;
    for (InstList::const_iterator I = CallSites.begin();
         I != CallSites.end(); ++I)
      AllCallers.push_back((*I)->getParent()->getParent());
    MakeUnique(AllCallers);
    if (!AllCallers.empty()) {
      O << "\t" << F->getName() << " is called by:\n";
      for (FuncList::iterator I = AllCallers.begin();
           I != AllCallers.end(); ++I) {
        O << "\t\t" << (*I)->getName() << "\n";
      }
    }
  }
}
