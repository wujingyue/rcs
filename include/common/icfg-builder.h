/**
 * Author: Jingyue
 *
 * Builds a conservative ICFG.
 * It's not context-sensitive. 
 */

#ifndef __ICFG_BUILDER_H
#define __ICFG_BUILDER_H

#include "llvm/Pass.h"
using namespace llvm;

#include "common/mbb.h"
#include "common/typedefs.h"
#include "common/icfg.h"

namespace rcs {
	struct ICFGBuilder: public ModulePass, public ICFG {
		static char ID;

		ICFGBuilder();
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
	};
}

#endif
