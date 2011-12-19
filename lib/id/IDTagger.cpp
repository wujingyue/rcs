/**
 * Author: Jingyue
 */

#define DEBUG_TYPE "tag-id"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/DerivedTypes.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/IDTagger.h"
#include "common/util.h"
using namespace rcs;

INITIALIZE_PASS(IDTagger, "tag-id",
		"Assign each instruction a unique ID", false, false)

STATISTIC(NumInstructions, "Number of instructions");

char IDTagger::ID = 0;

IDTagger::IDTagger(): ModulePass(ID) {
	initializeIDTaggerPass(*PassRegistry::getPassRegistry());
}

void IDTagger::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesCFG();
}

bool IDTagger::runOnModule(Module &M) {
	const IntegerType *IntType = IntegerType::get(M.getContext(), 32);
	for (Module::iterator F = M.begin(); F != M.end(); ++F) {
		for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
			for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
				Value *const InsID = ConstantInt::get(IntType, NumInstructions);
				I->setMetadata("ins_id", MDNode::get(M.getContext(), &InsID, 1));
				++NumInstructions;
			}
		}
	}
	return true;
}
