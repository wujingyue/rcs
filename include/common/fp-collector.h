/**
 * Author: Jingyue
 *
 * Reads the function pointer trace (fptrace).
 * Outputs the call edges involved in this trace. 
 */

#ifndef __RCS_FP_COLLECTOR_H
#define __RCS_FP_COLLECTOR_H

#include <vector>
using namespace std;

#include "llvm/Module.h"
#include "llvm/Pass.h"
using namespace llvm;

namespace rcs {
	struct FPCollector: public ModulePass {
		static char ID;

		FPCollector();
		bool runOnModule(Module &M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual void print(raw_ostream &O, const Module *M) const;

		unsigned get_num_call_edges() const { return call_edges.size(); }
		pair<Instruction *, Function *> get_call_edge(unsigned index) const {
			return call_edges[index];
		}

	private:
		vector<pair<Instruction *, Function *> > call_edges;
	};
}

#endif
