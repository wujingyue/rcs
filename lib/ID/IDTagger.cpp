#define DEBUG_TYPE "rcs-id"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
using namespace llvm;

#include "rcs/IDTagger.h"
#include "rcs/util.h"
using namespace rcs;

static RegisterPass<IDTagger> X("tag-id",
                                "Assign each instruction a unique ID",
                                false,
                                false);

STATISTIC(NumInstructions, "Number of instructions");

char IDTagger::ID = 0;

IDTagger::IDTagger(): ModulePass(ID) {}

void IDTagger::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

bool IDTagger::runOnModule(Module &M) {
  IntegerType *IntType = IntegerType::get(M.getContext(), 32);
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
        Constant *InsID = ConstantInt::get(IntType, NumInstructions);
        I->setMetadata("ins_id", MDNode::get(M.getContext(), InsID));
        ++NumInstructions;
      }
    }
  }
  return true;
}
