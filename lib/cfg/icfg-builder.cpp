/**
 * Author: Jingyue
 */

#include "llvm/Support/CFG.h"
#include "common/icfg-builder.h"
#include "common/callgraph-fp.h"
#include "common/util.h"
#include "common/InitializePasses.h"
using namespace llvm;

INITIALIZE_PASS_BEGIN(ICFGBuilder, "icfg",
		"Build inter-procedural control flow graph", false, true)
INITIALIZE_PASS_DEPENDENCY(MicroBasicBlockBuilder)
INITIALIZE_PASS_DEPENDENCY(CallGraphFP)
INITIALIZE_PASS_END(ICFGBuilder, "icfg",
		"Build inter-procedural control flow graph", false, true)

ICFGBuilder::ICFGBuilder(): ModulePass(ID) {
	initializeICFGBuilderPass(*PassRegistry::getPassRegistry());
}

void ICFGBuilder::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<MicroBasicBlockBuilder>();
	AU.addRequired<CallGraphFP>();
}

bool ICFGBuilder::runOnModule(Module &M) {
	MicroBasicBlockBuilder &MBBB = getAnalysis<MicroBasicBlockBuilder>();

	forallbb(M, bb) {
		for (mbb_iterator mi = MBBB.begin(bb), E = MBBB.end(bb); mi != E; ++mi)
			getOrInsertMBB(mi);
	}

	forallbb(M, bb) {
		for (mbb_iterator mi = MBBB.begin(bb), E = MBBB.end(bb); mi != E; ++mi) {
			// The ICFG will not contain any inter-thread edge. 
			// It's also difficult to handle them. How to deal with the return
			// edges? They are supposed to go to the pthread_join sites. 
			if (mi->end() != bb->end() && !is_pthread_create(mi->end())) {
				CallGraphFP &CG = getAnalysis<CallGraphFP>();
				FuncList callees = CG.get_called_functions(mi->end());
				bool calls_decl = false;
				for (size_t i = 0; i < callees.size(); ++i) {
					Function *callee = callees[i];
					if (callee->isDeclaration()) {
						calls_decl = true;
					} else {
						MicroBasicBlock *entry_mbb = MBBB.begin(callee->begin());
						addEdge(mi, entry_mbb);
					}
				}
				if (calls_decl) {
					mbb_iterator next_mbb = mi; ++next_mbb;
					addEdge(mi, next_mbb);
				}
			} else {
				for (succ_iterator si = succ_begin(bb); si != succ_end(bb); ++si) {
					MicroBasicBlock *succ_mbb = MBBB.begin(*si);
					addEdge(mi, succ_mbb);
				}
				TerminatorInst *ti = bb->getTerminator();
				if (is_ret(ti)) {
					CallGraphFP &CG = getAnalysis<CallGraphFP>();
					InstList call_sites = CG.get_call_sites(bb->getParent());
					for (size_t i = 0; i < call_sites.size(); ++i) {
						Instruction *call_site = call_sites[i];
						// Ignore inter-thread edges. 
						if (is_pthread_create(call_site))
							continue;
						MicroBasicBlock *next_mbb;
						if (isa<CallInst>(call_site)) {
							BasicBlock::iterator next = call_site;
							++next;
							next_mbb = MBBB.parent(next);
						} else {
							assert(isa<InvokeInst>(call_site));
							InvokeInst *inv = dyn_cast<InvokeInst>(call_site);
							if (isa<ReturnInst>(ti)) {
								next_mbb = MBBB.begin(inv->getNormalDest());
							} else {
								next_mbb = MBBB.begin(inv->getUnwindDest());
							}
						}
						addEdge(mi, next_mbb);
					}
				}
			}
		}
	}
	return false;
}

char ICFGBuilder::ID = 0;
