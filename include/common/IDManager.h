#ifndef __IDMANAGER_H
#define __IDMANAGER_H

/**
 * Author: Jingyue
 */

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Instruction.h"
#include "common/typedefs.h"
using namespace llvm;

namespace llvm {

	struct IDManager: public ModulePass {

		static char ID;
		static const unsigned INVALID_ID = (unsigned)-1;

		IDManager(): ModulePass(ID) {}
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
		virtual void print(raw_ostream &O, const Module *M) const;

		unsigned size() const { return IDMapping.size(); }
		unsigned getInstructionID(const Instruction *I) const;
		Instruction *getInstruction(unsigned InsID) const;
		InstList getInstructions(unsigned InsID) const;

	private:
		DenseMap<unsigned, InstList> IDMapping;
	};
}

#endif
