/**
 * Author: Jingyue
 */

#ifndef __ID_TAGGER_H
#define __ID_TAGGER_H

#include "llvm/Pass.h"
using namespace llvm;

namespace llvm {
	struct IDTagger: public ModulePass {
		static char ID;

		IDTagger(): ModulePass(ID) {}
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
	};
}

#endif
