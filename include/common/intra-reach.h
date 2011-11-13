/**
 * Author: Jingyue
 *
 * DEPRECATED: Use reach instead. It's more general. 
 */

#ifndef __INTRA_REACH_H
#define __INTRA_REACH_H

#include "llvm/Pass.h"
#include "common/typedefs.h"
using namespace llvm;

namespace llvm {
	/* TODO: Make it work with GraphTraits. Refer to DominatorTree */
	struct IntraReach: public FunctionPass {
		static char ID;

		IntraReach(): FunctionPass(&ID) {}
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnFunction(Function &F);
		/* Runs in linear time */
		bool reachable(const BasicBlock *x, const BasicBlock *y) const;
		/*
		 * Floodfill starting from <x>, ending with <sink>. 
		 * <visited> will contain all visited BBs. 
		 */
		void floodfill(
				const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const;
		void floodfill_r(
				const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const;
	};
}

#endif
