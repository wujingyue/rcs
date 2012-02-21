/**
 * Author: Jingyue
 */

#include <vector>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace rcs {
	struct ListFunctions: public ModulePass {
		static char ID;

		ListFunctions();
		virtual bool runOnModule(Module &M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	};
}
using namespace rcs;

static RegisterPass<ListFunctions> X("list-functions",
		"List all functions in a module", false, true);

char ListFunctions::ID = 0;

void ListFunctions::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
}

ListFunctions::ListFunctions(): ModulePass(ID) {}

bool ListFunctions::runOnModule(Module &M) {
	vector<string> names;
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		if (!f->isDeclaration())
			names.push_back(f->getName());
	}

	sort(names.begin(), names.end());
	for (size_t i = 0; i < names.size(); ++i)
		errs() << names[i] << "\n";

	return false;
}
