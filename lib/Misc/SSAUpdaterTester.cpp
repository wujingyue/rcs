#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"

using namespace llvm;

namespace rcs {
struct SSAUpdaterTester: public FunctionPass {
  static char ID;

  SSAUpdaterTester(): FunctionPass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function &F);
};
}

using namespace rcs;

char SSAUpdaterTester::ID = 0;

static RegisterPass<SSAUpdaterTester> X("test-ssaupdater",
                                        "Test SSAUpdater",
                                        false,
                                        false);

void SSAUpdaterTester::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
}

bool SSAUpdaterTester::runOnFunction(Function &F) {
  assert(F.size() == 3);

  Instruction *P = NULL;
  for (Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
    for (BasicBlock::iterator Ins = BB->begin(); Ins != BB->end(); ++Ins) {
      if (Ins->getName() == "p")
        P = Ins;
    }
  }
  assert(P);

  IntegerType *CharType = Type::getInt8Ty(F.getContext());
  PointerType *CharStarType = PointerType::getUnqual(CharType);
  Instruction *P2 = new BitCastInst(P, CharStarType, "p2",
                                    F.back().getFirstNonPHI());

  SSAUpdater SU;
  SU.Initialize(P->getType(), "");
  SU.AddAvailableValue(P->getParent(), P);
  if (P->getParent() != F.begin()) {
    SU.AddAvailableValue(F.begin(),
                         ConstantPointerNull::get(
                             cast<PointerType>(P->getType())));
  }
  SU.RewriteUse(P2->getOperandUse(0));

  return true;
}
