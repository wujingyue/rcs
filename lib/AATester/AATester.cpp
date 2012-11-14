#include "llvm/Argument.h"
#include "llvm/Function.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "rcs/IDAssigner.h"
using namespace rcs;

namespace rcs {
struct AATester: public ModulePass {
  static char ID;

  AATester(): ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

 private:
  static void PrintValue(raw_ostream &O, const Value *V);
};
}

static RegisterPass<AATester> X("test-aa",
                                "Test alias analysis",
                                false, // Is CFG Only?
                                true); // Is Analysis?

static cl::opt<bool> ValueID("value",
                             cl::desc("Use value IDs instead of "
                                      "instruction IDs"));
static cl::opt<unsigned> ID1("id1", cl::desc("the first ID"),
                             cl::init(IDAssigner::InvalidID));
static cl::opt<unsigned> ID2("id2", cl::desc("the second ID"),
                             cl::init(IDAssigner::InvalidID));

char AATester::ID = 0;

void AATester::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<AliasAnalysis>();
}

bool AATester::runOnModule(Module &M) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();
  AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

  const Value *V1 = NULL, *V2 = NULL;
  if (ValueID) {
    V1 = IDA.getValue(ID1);
    V2 = IDA.getValue(ID2);
  } else {
    Instruction *Ins1 = IDA.getInstruction(ID1);
    Instruction *Ins2 = IDA.getInstruction(ID2);
    assert(Ins1 && Ins2);
    assert(isa<StoreInst>(Ins1) || isa<LoadInst>(Ins1));
    assert(isa<StoreInst>(Ins2) || isa<LoadInst>(Ins2));
    if (isa<StoreInst>(Ins1))
      V1 = cast<StoreInst>(Ins1)->getPointerOperand();
    else
      V1 = cast<LoadInst>(Ins1)->getPointerOperand();
    if (isa<StoreInst>(Ins2))
      V2 = cast<StoreInst>(Ins2)->getPointerOperand();
    else
      V2 = cast<LoadInst>(Ins2)->getPointerOperand();
  }
  assert(V1 && V2);

  PrintValue(errs(), V1);
  errs() << "\n";
  PrintValue(errs(), V2);
  errs() << "\n";

  errs() << AA.alias(V1, V2) << "\n";
  return false;
}

void AATester::PrintValue(raw_ostream &O, const Value *V) {
  if (isa<Function>(V)) {
    O << V->getName();
  } else if (const Argument *Arg = dyn_cast<Argument>(V)) {
    O << Arg->getParent()->getName() << ":  " << *Arg;
  } else if (const Instruction *Ins = dyn_cast<Instruction>(V)) {
    O << Ins->getParent()->getParent()->getName();
    O << "." << Ins->getParent()->getName() << ":";
    O << *Ins;
  } else {
    O << *V;
  }
}
