/**
 * Author: Jingyue
 *
 * Most effective on max-sliced programs, but general enough to work with
 * any program. 
 *
 * Only builds the ICFG for the functions that have only one calling context.
 * The ICFG will not contain inter-thread edges (i.e. pthread_create call). 
 *
 * We create a fake root points to the entry of each thread (including
 * the main thread), because DominatorTreeBase only supports CFGs with
 * only one root. 
 */

#ifndef __PARTIAL_ICFG_BUILDER_H
#define __PARTIAL_ICFG_BUILDER_H

#include "llvm/Pass.h"
#include "common/icfg.h"
#include "common/mbb.h"
using namespace llvm;

namespace llvm {
	struct PartialICFGBuilder: public ModulePass, public ICFG {
		static char ID;

		PartialICFGBuilder();
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
		/** 
		 * This method is used when a pass implements
		 * an analysis interface through multiple inheritance.  If needed, it
		 * should override this to adjust the this pointer as needed for the
		 * specified pass info.
		 */
		virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {   
			if (PI == &ICFG::ID)
				return (ICFG *)this;
			return this;
		}

	private:
		/*
		 * Decide whether we should trace into <ins>.
		 * Returns NULL if we shouldn't. 
		 * We trace into <ins> iff:
		 * 1. <ins> is a call instruction. We don't consider pthread_create calls
		 *    the thread function. 
		 * 2. It has only one callee.
		 * 3. The callee isn't a declaration. 
		 * 4. The callee can be executed only once. 
		 */
		Function *trace_into(Instruction *ins);
		void dump_icfg(Module &M);
	};
}

#endif
