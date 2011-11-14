#ifndef __IDENTIFY_THREAD_FUNCS_H
#define __IDENTIFY_THREAD_FUNCS_H

#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "common/typedefs.h"
using namespace llvm;

namespace llvm {
	struct IdentifyThreadFuncs: public ModulePass {
		static char ID;

		IdentifyThreadFuncs(): ModulePass(ID) {}

		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
		virtual void print(raw_ostream &O, const Module *M) const;
		bool is_thread_func(Function *f) const;

	private:
		FuncSet thread_funcs;
	};
}

#endif
