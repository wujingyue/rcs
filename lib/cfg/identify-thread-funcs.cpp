#include "common/InitializePasses.h"
using namespace llvm;

#include "common/callgraph-fp.h"
#include "common/identify-thread-funcs.h"
#include "common/util.h"
using namespace rcs;

INITIALIZE_PASS_BEGIN(IdentifyThreadFuncs, "identify-thread-funcs",
		"Identify thread functions", false, true)
INITIALIZE_PASS_DEPENDENCY(CallGraphFP)
INITIALIZE_PASS_END(IdentifyThreadFuncs, "identify-thread-funcs",
		"Identify thread functions", false, true)

char IdentifyThreadFuncs::ID = 0;

void IdentifyThreadFuncs::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<CallGraphFP>();
}

IdentifyThreadFuncs::IdentifyThreadFuncs(): ModulePass(ID) {
	initializeIdentifyThreadFuncsPass(*PassRegistry::getPassRegistry());
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
					CallGraphFP &CG = getAnalysis<CallGraphFP>();
					FuncList callees = CG.get_called_functions(ii);
					for (size_t i = 0; i < callees.size(); ++i) {
						if (callees[i]->getName() != "pthread_create")
							thread_funcs.insert(callees[i]);
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
