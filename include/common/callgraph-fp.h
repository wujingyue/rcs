/**
 * Author: Jingyue
 *
 * Fit into the CallGraph interface, so that we can run graph algorithms
 * (e.g. SCC) on it. 
 */

#ifndef __CALLGRAPH_FP_H
#define __CALLGRAPH_FP_H

#include <vector>
#include <set>
using namespace std;

#include "llvm/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/DenseMap.h"
#include "common/typedefs.h"
using namespace llvm;

namespace llvm {
	struct CallGraphFP: public ModulePass, public CallGraph {
		typedef DenseMap<const Instruction *, FuncList> SiteFuncMapping;
		typedef DenseMap<const Function *, InstList> FuncSiteMapping;

		static char ID;

		explicit CallGraphFP(char *ID):
			ModulePass(ID), root(NULL), extern_calling_node(NULL),
			calls_extern_node(NULL) {}

		CallGraphFP():
			ModulePass(&ID), root(NULL), extern_calling_node(NULL),
			calls_extern_node(NULL) {}

		// Interfaces of ModulePass
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
		virtual void print(raw_ostream &O, const Module *M) const;
		/** 
		 * This method is used when a pass implements
		 * an analysis interface through multiple inheritance.  If needed, it
		 * should override this to adjust the this pointer as needed for the
		 * specified pass info.
		 */
		virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {   
			if (PI->isPassID(&CallGraph::ID))
				return (CallGraph*)this;
			return this;
		}
		// Interfaces of CallGraph
		const CallGraphNode *getRoot() const { return root; }
		CallGraphNode *getRoot() { return root; }
		// SCC algorithm starts from this external calling node. 
		CallGraphNode *getExternalCallingNode() const {
			return extern_calling_node;
		}
		CallGraphNode *getCallsExternalNode() const { return calls_extern_node; }
		virtual void destroy();

		FuncList get_called_functions(const Instruction *ins) const;
		InstList get_call_sites(const Function *f) const;

	protected:
		void add_call_edge(const CallSite &cs, Function *callee);
		template <class T> void make_unique(vector<T> &v);

	private:
		void process_call_site(const CallSite &cs, set<const Function *> &all_funcs);
		void simplify_call_graph();
		void add_extra_call_edges(Module &M);

		SiteFuncMapping called_funcs;
		FuncSiteMapping call_sites;

		CallGraphNode *root;
		CallGraphNode *extern_calling_node;
		CallGraphNode *calls_extern_node;
	};
}

#endif
