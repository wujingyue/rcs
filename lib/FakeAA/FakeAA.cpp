#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasAnalysis.h"
using namespace llvm;

#include "rcs/IDAssigner.h"
using namespace rcs;

namespace rcs {
struct FakeAA: public ImmutablePass, public AliasAnalysis {
  static char ID;

  FakeAA(): ImmutablePass(ID) {}
  virtual void initializePass();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void *getAdjustedAnalysisPointer(AnalysisID PI);

  // interfaces of AliasAnalysis
  virtual AliasResult alias(const Location &L1, const Location &L2) {
    IDAssigner &IDA = getAnalysis<IDAssigner>();
    errs() << "FakeAA::alias " << IDA.getNumValues() << "\n";
    return NoAlias;
  }
  virtual ModRefBehavior getModRefBehavior(const Function *F) { return DoesNotAccessMemory; }
  virtual ModRefBehavior getModRefBehavior(ImmutableCallSite CS) { return DoesNotAccessMemory; }
  virtual bool pointsToConstantMemory(const Location &Loc, bool OrLocal = false) { return true; }
  virtual void deleteValue(Value *V) {}
  virtual void copyValue(Value *From, Value *To) {}
  virtual void addEscapingUse(Use &U) {}
};
}

static RegisterPass<FakeAA> X("fake-aa", "Fake AA", false, true);
static RegisterAnalysisGroup<AliasAnalysis> Y(X);

char FakeAA::ID = 0;

void FakeAA::initializePass() {
  DEBUG(dbgs() << "FakeAA::initializePass\n";);
  InitializeAliasAnalysis(this);
}

void FakeAA::getAnalysisUsage(AnalysisUsage &AU) const {
  AliasAnalysis::getAnalysisUsage(AU);
  AU.addRequired<IDAssigner>();
}

void *FakeAA::getAdjustedAnalysisPointer(AnalysisID PI) {
  if (PI == &AliasAnalysis::ID)
    return (AliasAnalysis *)this;
  return this;
}
