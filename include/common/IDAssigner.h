/**
 * Author: Jingyue
 */

#ifndef __ID_ASSIGNER_H
#define __ID_ASSIGNER_H

#include "llvm/Pass.h"
#include "llvm/Instruction.h"
#include "llvm/ADT/DenseMap.h"
using namespace llvm;

namespace llvm {
	struct IDAssigner: public ModulePass {
		static char ID;
		static const unsigned INVALID_ID = -1;

		IDAssigner();
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module& M);
		virtual void print(raw_ostream &O, const Module *M) const;
		void printInstructions(raw_ostream &O, const Module *M) const;
		void printValues(raw_ostream &O, const Module *M) const;

		unsigned getInstructionID(const Instruction *I) const;
		unsigned getValueID(const Value *V) const;
		unsigned getFunctionID(const Function *F) const;
		Instruction *getInstruction(unsigned ID) const;
		Value *getValue(unsigned ID) const;
		Function *getFunction(unsigned ID) const;

	private:
		bool addValue(Value *V);
		bool addIns(Instruction *I);
		bool addFunction(Function *F);
		void extractValuesInUser(User *U);
		void printValue(raw_ostream &O, const Value *V) const;

		DenseMap<const Instruction *, unsigned> InsIDMapping;
		DenseMap<const Value *, unsigned > ValueIDMapping;
		DenseMap<unsigned, Instruction *> IDInsMapping;
		DenseMap<unsigned, Value *> IDValueMapping;
		DenseMap<const Function *, unsigned> FunctionIDMapping;
		DenseMap<unsigned, Function *> IDFunctionMapping;
	};
}

#endif
