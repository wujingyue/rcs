// Author: Jingyue

// The default implementation of PointerAnalysis. 

#include <cstdio>
#include <list>
#include <string>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/IDAssigner.h"
#include "common/PointerAnalysis.h"
using namespace rcs;

namespace rcs {
struct BasicPointerAnalysis: public ModulePass, public PointerAnalysis {
  static char ID;

  BasicPointerAnalysis();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void getPointees(const Value *Pointer, ValueList &Pointees);
  virtual void print(raw_ostream &O, const Module *M) const;
  // A very important function. Otherwise getAnalysis<PointerAnalysis> would
  // not be able to return BasicPointerAnalysis. 
  virtual void *getAdjustedAnalysisPointer(AnalysisID PI);
  
 private:
  bool isMallocCall(Value *V) const;
  bool isMalloc(Function *F) const;

  vector<string> MallocNames;
  // Leader[V] is the leader of the equivalence class <V> belongs to. 
  // We could use llvm::EquivalenceClasses here. 
  ConstValueMapping Leader;
  // Allocators[L] is the set of all allocators in the set leaded by L. 
  // Allocators[V] does not make sense for a value that's not a leader. 
  DenseMap<const Value *, ValueList> Allocators;
};
}

char BasicPointerAnalysis::ID = 0;

#if 0
static RegisterAnalysisGroup<PointerAnalysis, true> Y(X);
#endif

INITIALIZE_AG_PASS_BEGIN(BasicPointerAnalysis, PointerAnalysis, "basicpa",
                         "Basic Pointer Analysis", false, true, true)
INITIALIZE_PASS_DEPENDENCY(IDAssigner)
INITIALIZE_AG_DEPENDENCY(AliasAnalysis)
INITIALIZE_AG_PASS_END(BasicPointerAnalysis, PointerAnalysis, "basicpa",
                       "Basic Pointer Analysis", false, true, true)
#if 0
static RegisterPass<BasicPointerAnalysis> X("basicpa",
                                            "Basic Pointer Analysis",
                                            false, // Is CFG Only? 
                                            true); // Is Analysis? 
#endif
void BasicPointerAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<AliasAnalysis>();
}

BasicPointerAnalysis::BasicPointerAnalysis(): ModulePass(ID) {
  initializeBasicPointerAnalysisPass(*PassRegistry::getPassRegistry());
  // Initialize the list of memory allocatores.
  MallocNames.push_back("malloc");
  MallocNames.push_back("calloc");
  MallocNames.push_back("valloc");
  MallocNames.push_back("realloc");
  MallocNames.push_back("memalign");
  MallocNames.push_back("_Znwm");
  MallocNames.push_back("_Znaj");
  MallocNames.push_back("_Znam");
}

bool BasicPointerAnalysis::runOnModule(Module &M) {
  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
  IDAssigner &IDA = getAnalysis<IDAssigner>();

  list<Value *> RemainingValues;
  for (unsigned i = 0; i < IDA.getNumValues(); ++i)
    RemainingValues.push_back(IDA.getValue(i));

  while (!RemainingValues.empty()) {
    Value *TheLeader = RemainingValues.front();
    for (list<Value *>::iterator I = RemainingValues.begin();
         I != RemainingValues.end(); ) {
      Value *V = *I;
      if (AA.alias(TheLeader, 0, V, 0) != AliasAnalysis::NoAlias) {
        Leader[V] = TheLeader;
        if (isa<GlobalValue>(V) || isa<AllocaInst>(V) || isMallocCall(V))
          Allocators[TheLeader].push_back(V); 
        list<Value *>::iterator ToDelete = I;
        ++I;
        RemainingValues.erase(ToDelete);
      } else {
        ++I;
      }
    }
  }
  dbgs() << "# of equivalence classes = " << Allocators.size() << "\n";
  
  return false;
}

void BasicPointerAnalysis::getPointees(const Value *Pointer,
                                       ValueList &Pointees) {
  assert(Leader.count(Pointer) && "<Pointer> is not a captured value");
  const Value *TheLeader = Leader.lookup(Pointer);

  Pointees.clear();
  DenseMap<const Value *, ValueList>::const_iterator I =
      Allocators.find(TheLeader);
  if (I != Allocators.end())
    Pointees = I->second;
}

bool BasicPointerAnalysis::isMallocCall(Value *V) const {
  Instruction *I = dyn_cast<Instruction>(V);
  if (I == NULL)
    return false;

  CallSite CS(I);
  if (CS.getInstruction() == NULL)
    return false;

  Function *Callee = CS.getCalledFunction();
  return Callee && isMalloc(Callee);
}

bool BasicPointerAnalysis::isMalloc(Function *F) const {
  vector<string>::const_iterator Pos = find(MallocNames.begin(),
                                            MallocNames.end(),
                                            F->getName());
  return Pos != MallocNames.end();
}

void BasicPointerAnalysis::print(raw_ostream &O, const Module *M) const {
}

void *BasicPointerAnalysis::getAdjustedAnalysisPointer(AnalysisID PI) {
  if (PI == &PointerAnalysis::ID)
    return (PointerAnalysis *)this;
  return this;
}
