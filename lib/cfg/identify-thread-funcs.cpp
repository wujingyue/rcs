#include "common/identify-thread-funcs.h"
#include "common/util.h"
using namespace llvm;

#include "bc2bdd/BddAliasAnalysis.h"
using namespace repair;

static RegisterPass<IdentifyThreadFuncs> X("identify-thread-funcs",
		"Identify thread functions",
		false, true); // is analysis

char IdentifyThreadFuncs::ID = 0;

void IdentifyThreadFuncs::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<BddAliasAnalysis>();
	ModulePass::getAnalysisUsage(AU);
}

bool IdentifyThreadFuncs::runOnModule(Module &M) {
	/*
	 * Get the set of all defined functions.
	 * Will be used as a candidate set for point-to analysis. 
	 */
	set<const Function *> all_defined_funcs;
	forallfunc(M, fi) {
		if (!fi->isDeclaration())
			all_defined_funcs.insert(fi);
	}
	/*
	 * Extract the called thread functions for each pthread_create.
	 */
	thread_funcs.clear();
	forallinst(M, ii) {
		if (is_pthread_create(ii)) {
			if (Value *thr_func = get_pthread_create_callee(ii)) {
				if (isa<Function>(thr_func)) {
					// pthread_create with a known function
					thread_funcs.insert(cast<Function>(thr_func));
				} else {
					BddAliasAnalysis &BAA = getAnalysis<BddAliasAnalysis>();
					// Ask BAA which defined functions <fp> may point to. 
					set<const Value *> pointees;
					BAA.pointsTo(thr_func, 0, &all_defined_funcs, pointees);
					forall(set<const Value *>, it, pointees) {
						thread_funcs.insert(
								const_cast<Function *>(dyn_cast<Function>(*it)));
					}
				}
			}
		}
	}
	return false;
}

void IdentifyThreadFuncs::print(raw_ostream &O, const Module *M) const {
	O << "Thread functions:\n";
	forallconst(FuncSet, it, thread_funcs)
		O << (*it)->getNameStr() << "\n";
}

bool IdentifyThreadFuncs::is_thread_func(Function *f) const {
	return thread_funcs.count(f);
}
