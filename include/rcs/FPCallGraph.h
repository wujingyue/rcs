// Fit into the CallGraph interface, so that we can run graph algorithms
// (e.g. SCC) on it.

#ifndef __RCS_FP_CALLGRAPH_H
#define __RCS_FP_CALLGRAPH_H

#include <vector>

#include "llvm/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/DenseMap.h"

#include "rcs/typedefs.h"

using namespace llvm;

namespace rcs {
struct FPCallGraph: public ModulePass, public CallGraph {
  typedef DenseMap<const Instruction *, FuncList> SiteToFuncsMapTy;
  typedef DenseMap<const Function *, InstList> FuncToSitesMapTy;

  static char ID;

  // Interfaces of ModulePass
  FPCallGraph();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &O, const Module *M) const;
  // This method is used when a pass implements
  // an analysis interface through multiple inheritance.  If needed, it
  // should override this to adjust the this pointer as needed for the
  // specified pass info.
  virtual void *getAdjustedAnalysisPointer(AnalysisID PI);

  // Interfaces of CallGraph
  const CallGraphNode *getRoot() const { return Root; }
  CallGraphNode *getRoot() { return Root; }
  // SCC algorithm starts from this external calling node.
  CallGraphNode *getExternalCallingNode() const {
    return ExternCallingNode;
  }
  CallGraphNode *getCallsExternalNode() const { return CallsExternNode; }
  virtual void destroy();

  FuncList getCalledFunctions(const Instruction *Ins) const;
  InstList getCallSites(const Function *F) const;

 protected:
  void addCallEdge(const CallSite &CS, Function *Callee);

 private:
  template <typename T>
  static void MakeUnique(std::vector<T> &V);

  void processCallSite(const CallSite &CS, const FuncSet &AllFuncs);
  void simplifyCallGraph();

  SiteToFuncsMapTy SiteToFuncs;
  FuncToSitesMapTy FuncToSites;

  CallGraphNode *Root;
  CallGraphNode *ExternCallingNode;
  CallGraphNode *CallsExternNode;
};
}

#endif
