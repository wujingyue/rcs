/**
 * Author: Jingyue
 */

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

namespace rcs {
	struct IdentifyLoops: public ModulePass {
		static char ID;
		IdentifyLoops();
		virtual bool runOnModule(Module &M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	};
}
using namespace rcs;

static RegisterPass<IdentifyLoops> X("identify-loops",
		"Identify loops", false, false);

char IdentifyLoops::ID = 0;

void IdentifyLoops::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<AliasAnalysis>();
}

IdentifyLoops::IdentifyLoops(): ModulePass(ID) {}

bool IdentifyLoops::runOnModule(Module &M) {
	AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
	errs() << (void *)&AA.ID << "\n";
	Value *the_v = NULL;
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
			for (BasicBlock::iterator ins = bb->begin(); ins != bb->end(); ++ins) {
				if (isa<PointerType>(ins->getType())) {
					the_v = ins;
					break;
				}
			}
		}
	}
	assert(the_v);
	errs() << AA.alias(the_v, the_v) << "\n";
	return true;
}
