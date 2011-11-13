/**
 * Author: Jingyue
 */

#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"
#include "common/partial-icfg-builder.h"
#include "common/exec-once.h"
#include "common/callgraph-fp.h"
#include "common/util.h"
using namespace llvm;

#include "bc2bdd/BddAliasAnalysis.h"
using namespace repair;

static RegisterPass<PartialICFGBuilder> X(
		"partial-icfg-builder", "Builds part of the ICFG",
		false, true); // is an analysis pass

static cl::opt<bool> DumpICFG("dump-icfg",
		cl::desc("Dump the ICFG"));

char PartialICFGBuilder::ID = 0;

void PartialICFGBuilder::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<MicroBasicBlockBuilder>();
	AU.addRequired<BddAliasAnalysis>();
	AU.addRequired<CallGraphFP>();
	AU.addRequired<ExecOnce>();
	ModulePass::getAnalysisUsage(AU);
}

void PartialICFGBuilder::dump_icfg(Module &M) {
	ICFG::print(errs());
}

bool PartialICFGBuilder::runOnModule(Module &M) {

	// Find the main function. To be used later. 
	Function *main = NULL;
	forall(Module, fi, M) {
		if (is_main(fi)) {
			main = fi;
			break;
		}
	}
	assert(main && "Cannot find the main function.");

	// Find all MBBs that are in an executed-only-once function.
	// Create a ICFG node for each of them. 
	ExecOnce &EO = getAnalysis<ExecOnce>();
	MicroBasicBlockBuilder &MBBB = getAnalysis<MicroBasicBlockBuilder>();
	forallfunc(M, f) {
		if (f->isDeclaration() || EO.not_executed(f) || !EO.executed_once(f))
			continue;
		forall(Function, bb, *f) {
			for (mbb_iterator mi = MBBB.begin(bb); mi != MBBB.end(bb); ++mi)
				getOrInsertMBB(mi);
		}
	}

	// Link these MBBs. 
	for (ICFG::iterator ii = begin(); ii != end(); ++ii) {
		MicroBasicBlock *x = ii->getMBB();
		BasicBlock *bb = x->getParent();
		Instruction *last = &x->back();
		Function *callee = trace_into(last);
		if (callee) {
			MicroBasicBlock *y = MBBB.begin(&callee->getEntryBlock());
			if ((*this)[y] != NULL)
				addEdge(x, y);
		} else if (x->end() != bb->end()) {
			mbb_iterator y = x; ++y;
			addEdge(x, y);
		} else {
			assert(x->end() == bb->end());
			for (succ_iterator si = succ_begin(bb); si != succ_end(bb); ++si) {
				MicroBasicBlock *y = MBBB.begin(*si);
				addEdge(x, y);
			}
			// The main function returns to nowhere. 
			if (is_ret(last) && bb->getParent() != main) {
				CallGraphFP &CG = getAnalysis<CallGraphFP>();
				InstList call_sites = CG.get_call_sites(bb->getParent());
				unsigned n_reachable_call_sites = 0;
				Instruction *the_call_site = NULL;
				for (size_t j = 0; j < call_sites.size(); ++j) {
					if (!EO.not_executed(call_sites[j])) {
						n_reachable_call_sites++;
						the_call_site = call_sites[j];
					}
				}
				if (n_reachable_call_sites != 1) {
					errs() << "Function " << bb->getParent()->getName() << " has " <<
						call_sites.size() << " reachable call sites.\n";
				}
				assert(n_reachable_call_sites == 1);
				// We don't add inter-thread edges. 
				if (!is_pthread_create(the_call_site)) {
					MicroBasicBlock *y;
					if (isa<CallInst>(the_call_site)) {
						BasicBlock::iterator next = the_call_site;
						++next;
						y = MBBB.parent(next);
					} else {
						assert(isa<InvokeInst>(the_call_site));
						InvokeInst *inv = dyn_cast<InvokeInst>(the_call_site);
						y = (isa<ReturnInst>(last) ?
								MBBB.begin(inv->getNormalDest()) :
								MBBB.begin(inv->getUnwindDest()));
					}
					addEdge(x, y);
				}
			} // is ret
		} // is callee
	}

	// Add a fake root, because DominatorTreeBase only supports one root. 
	vector<MicroBasicBlock *> current_roots;
	for (ICFG::iterator ii = begin(); ii != end(); ++ii) {
		if (ii->pred_begin() == ii->pred_end())
			current_roots.push_back(ii->getMBB());
	}
	for (size_t j = 0; j < current_roots.size(); ++j)
		addEdge(NULL, current_roots[j]);

	if (DumpICFG)
		dump_icfg(M);

	return false;
}

Function *PartialICFGBuilder::trace_into(Instruction *ins) {
	if (!is_call(ins))
		return NULL;
	if (is_pthread_create(ins))
		return NULL;
	CallGraphFP &CG = getAnalysis<CallGraphFP>();
	FuncList callees = CG.get_called_functions(ins);
	if (callees.size() != 1)
		return NULL;
	Function *callee = callees[0];
	if (callee->isDeclaration())
		return NULL;
	if (!getAnalysis<ExecOnce>().executed_once(callee))
		return NULL;
	return callee;
}
