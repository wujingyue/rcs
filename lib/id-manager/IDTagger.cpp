/**
 * Author: Jingyue
 */

#define DEBUG_TYPE "tag-id"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/DerivedTypes.h"
#include "common/IDTagger.h"
#include "common/util.h"
using namespace llvm;

static RegisterPass<IDTagger> X("tag-id",
		"Assign each instruction a unique ID");

STATISTIC(NumInstructions, "Number of instructions");

char IDTagger::ID = 0;

void IDTagger::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesCFG();
	ModulePass::getAnalysisUsage(AU);
}

bool IDTagger::runOnModule(Module &M) {
	const IntegerType *IntType = IntegerType::get(M.getContext(), 32);
	forallinst(M, I) {
		Value *const InsID = ConstantInt::get(IntType, NumInstructions);
		I->setMetadata("ins_id", MDNode::get(M.getContext(), &InsID, 1));
		++NumInstructions;
	}
	return true;
}
