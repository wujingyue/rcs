/**
 * Author: Jingyue
 *
 * Reads the function pointer trace (fptrace).
 * Outputs the call edges involved in this trace. 
 */

#define DEBUG_TYPE "fp-collector"

#include <cassert>
#include <fstream>
#include <sstream>
#include <map>
using namespace std;

#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/util.h"
#include "common/fp-collector.h"
#include "common/IDManager.h"
using namespace rcs;

static cl::opt<string> FPTrace("fptrace",
		cl::desc("Function pointer trace"));

FPCollector::FPCollector(): ModulePass(ID) {
	initializeFPCollectorPass(*PassRegistry::getPassRegistry());
}

INITIALIZE_PASS_BEGIN(FPCollector, "collect-fp",
		"Collect calls via function pointers in the fp trace", false, true)
INITIALIZE_PASS_DEPENDENCY(IDManager)
INITIALIZE_PASS_END(FPCollector, "collect-fp",
		"Collect calls via function pointers in the fp trace", false, true)

void FPCollector::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<IDManager>();
}

char FPCollector::ID = 0;

bool FPCollector::runOnModule(Module &M) {
	if (FPTrace == "") {
		errs() << "[Warning] Didn't specify any extra call edges\n";
		return false;
	}

	ifstream fin(FPTrace.c_str());
	pthread_t tid;
	string func_name;
	map<pthread_t, unsigned> current_call_ins_id;
	while (fin >> tid >> func_name) {
		if (isdigit(func_name[0])) {
			istringstream iss(func_name);
			unsigned ins_id;
			iss >> ins_id;
			current_call_ins_id[tid] = ins_id;
		} else {
			map<pthread_t, unsigned>::iterator it = current_call_ins_id.find(tid);
			if (it != current_call_ins_id.end() && it->second != (unsigned)-1) {
				IDManager &IDM = getAnalysis<IDManager>();
				Instruction *ci = IDM.getInstruction(it->second);
				assert(ci && is_call(ci));
				Function *callee = M.getFunction(func_name);
				assert(callee);
				call_edges.push_back(make_pair(ci, callee));
				it->second = (unsigned)-1;
			}
		}
	}

	return false;
}

void FPCollector::print(raw_ostream &O, const Module *M) const {
	// Do nothing. 
}
