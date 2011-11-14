/**
 * Author: Jingyue
 *
 * Builds a conservative ICFG.
 * It's not context-sensitive. 
 */

#ifndef __ICFG_BUILDER_H
#define __ICFG_BUILDER_H

#include "llvm/Pass.h"
#include "common/mbb.h"
#include "common/typedefs.h"
#include "common/icfg.h"

namespace llvm {
	struct ICFGBuilder: public ModulePass, public ICFG {
		static char ID;

		ICFGBuilder(): ModulePass(ID) {}
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
	};
}

#endif
