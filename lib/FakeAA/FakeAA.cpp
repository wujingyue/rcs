#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"
using namespace llvm;

namespace rcs {
struct FakeAA: public ModulePass, public AliasAnalysis {
  static char ID;

  FakeAA(): ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void *getAdjustedAnalysisPointer(AnalysisID PI);
  virtual AliasResult alias(const Location &L1, const Location &L2);
};
}
using namespace rcs;

static RegisterPass<FakeAA> X("fake-aa", "Fake AA", false, true);
static RegisterAnalysisGroup<AliasAnalysis> Y(X);

char FakeAA::ID = 0;

bool FakeAA::runOnModule(Module &M) {
  errs() << "FakeAA::runOnModule\n";
  InitializeAliasAnalysis(this);
  return false;
}

void FakeAA::getAnalysisUsage(AnalysisUsage &AU) const {
  AliasAnalysis::getAnalysisUsage(AU);
  AU.setPreservesAll();
}

void *FakeAA::getAdjustedAnalysisPointer(AnalysisID PI) {
  if (PI == &AliasAnalysis::ID)
    return (AliasAnalysis *)this;
  return this;
}

AliasAnalysis::AliasResult FakeAA::alias(const Location &L1,
                                         const Location &L2) {
  return AliasAnalysis::alias(L1, L2);
}
