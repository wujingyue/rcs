// The default implementation of PointerAnalysis. 

#include <cstdio>
#include <list>
#include <string>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

#include "rcs/IDAssigner.h"
#include "rcs/PointerAnalysis.h"
using namespace rcs;

namespace rcs {
struct BasicPointerAnalysis: public ModulePass, public PointerAnalysis {
  static char ID;

  BasicPointerAnalysis();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

  virtual void getAllPointers(ValueList &Pointers);
  virtual bool getPointees(const Value *Pointer, ValueList &Pointees);

  // A very important function. Otherwise getAnalysis<PointerAnalysis> would
  // not be able to return BasicPointerAnalysis. 
  virtual void *getAdjustedAnalysisPointer(AnalysisID PI);
  
 private:
  bool isMallocCall(Value *V) const;
  bool isMalloc(Function *F) const;
  bool shouldFilterOut(Value *V) const;

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

static RegisterPass<BasicPointerAnalysis> X("basic-pa",
                                            "Basic Pointer Analysis",
                                            false, // Is CFG Only? 
                                            true); // Is Analysis? 
static RegisterAnalysisGroup<PointerAnalysis, true> Y(X);

void BasicPointerAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<AliasAnalysis>();
}

BasicPointerAnalysis::BasicPointerAnalysis(): ModulePass(ID) {
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

bool BasicPointerAnalysis::shouldFilterOut(Value *V) const {
  if (Argument *Arg = dyn_cast<Argument>(V)) {
    if (Arg->getParent()->isDeclaration())
      return true;
  }
  return false;
}

bool BasicPointerAnalysis::runOnModule(Module &M) {
  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

  ValueList AllPointers;
  getAllPointers(AllPointers);
  list<Value *> RemainingPointers(AllPointers.begin(), AllPointers.end());

  // Note that pointers and pointees are all of PointerType. 
  while (!RemainingPointers.empty()) {
    Value *TheLeader = RemainingPointers.front();
    for (list<Value *>::iterator I = RemainingPointers.begin();
         I != RemainingPointers.end(); ) {
      Value *V = *I;
      // alias(V1, 0, V2, 0) would always return NoAlias, because the ranges
      // are zero-size and thus disjoint. 
      if (AA.alias(TheLeader, 1, V, 1) != AliasAnalysis::NoAlias) {
        Leader[V] = TheLeader;
        if (isa<GlobalValue>(V) || isa<AllocaInst>(V) || isMallocCall(V))
          Allocators[TheLeader].push_back(V); 
        list<Value *>::iterator ToDelete = I;
        ++I;
        RemainingPointers.erase(ToDelete);
      } else {
        ++I;
      }
    }
  }
  dbgs() << "# of equivalence classes = " << Allocators.size() << "\n";
  
  return false;
}

void BasicPointerAnalysis::getAllPointers(ValueList &Pointers) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();

  Pointers.clear();
  for (unsigned i = 0; i < IDA.getNumValues(); ++i) {
    Value *V = IDA.getValue(i);
    if (V->getType()->isPointerTy() && !shouldFilterOut(V))
      Pointers.push_back(V);
  }
}

bool BasicPointerAnalysis::getPointees(const Value *Pointer,
                                       ValueList &Pointees) {
  assert(Pointer->getType()->isPointerTy() && "<Pointer> is not a pointer");

  if (!Leader.count(Pointer))
    return false;
  const Value *TheLeader = Leader.lookup(Pointer);

  Pointees.clear();
  DenseMap<const Value *, ValueList>::const_iterator I =
      Allocators.find(TheLeader);
  if (I != Allocators.end())
    Pointees = I->second;

  return true;
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

void *BasicPointerAnalysis::getAdjustedAnalysisPointer(AnalysisID PI) {
  if (PI == &PointerAnalysis::ID)
    return (PointerAnalysis *)this;
  return this;
}
