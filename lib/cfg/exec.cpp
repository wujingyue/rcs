/**
 * Author: Jingyue
 */

#include "llvm/Support/CFG.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SCCIterator.h"
#include "common/exec.h"
#include "common/callgraph-fp.h"
#include "common/util.h"
#include "common/reach.h"
#include "common/InitializePasses.h"
using namespace llvm;

char Exec::ID = 0;

INITIALIZE_PASS_BEGIN(Exec, "exec",
		"Test whether a function may or must execute a landmark", false, true)
INITIALIZE_PASS_DEPENDENCY(CallGraphFP)
INITIALIZE_PASS_END(Exec, "exec",
		"Test whether a function may or must execute a landmark", false, true)

Exec::Exec(): ModulePass(ID) {
	initializeExecPass(*PassRegistry::getPassRegistry());
}

void Exec::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequiredTransitive<CallGraphFP>();
}

void Exec::setup_landmarks(const ConstInstSet &landmarks) {
	this->landmarks = landmarks;
}

bool Exec::is_landmark(const Instruction *ins) const {
	return landmarks.count(ins);
}

void Exec::dfs(const Function *f, ConstFuncMapping &parent) {
	CallGraphFP &CG = getAnalysis<CallGraphFP>();
	const InstList &call_sites = CG.get_call_sites(f);
	for (size_t i = 0; i < call_sites.size(); ++i) {
		Function *caller = call_sites[i]->getParent()->getParent();
		if (!parent.count(caller)) {
			parent[caller] = f;
			dfs(caller, parent);
		}
	}
}

bool Exec::may_exec_landmark(const Function *f) const {
	return parent.count(f);
}

void Exec::traverse_call_graph() {
	parent.clear();
	for (ConstInstSet::iterator it = landmarks.begin(); it != landmarks.end();
			++it) {
		const Function *start = (*it)->getParent()->getParent();
		parent[start] = start;
		dfs(start, parent);
	}
}

void Exec::print_call_chain(const Function *f) {
	if (!parent.count(f)) {
		errs() << "\n";
		return;
	}
	errs() << f->getNameStr() << " => ";
	print_call_chain(parent[f]);
}

bool Exec::runOnModule(Module &M) {
	return false;
}

void Exec::compute_must_exec() {
	CallGraph &CG = getAnalysis<CallGraphFP>();
	for (scc_iterator<CallGraph *> si = scc_begin(&CG), E = scc_end(&CG);
			si != E; ++si) {
		for (size_t i = 0; i < (*si).size(); ++i) {
			if (Function *f = (*si)[i]->getFunction()) {
				if (f && !f->isDeclaration() && compute_must_exec(f))
					must_exec.insert(f);
			}
		}
	}
}

bool Exec::must_exec_landmark(const Function *f) const {
	return must_exec.count(f);
}

bool Exec::must_exec_landmark(const BasicBlock *bb) const {
	for (BasicBlock::const_iterator ins = bb->begin(); ins != bb->end(); ++ins) {
		if (must_exec_landmark(ins))
			return true;
	}
	return false;
}

bool Exec::may_exec_landmark(const Instruction *ins) const {
	if (is_landmark(ins))
		return true;
	if (!is_call(ins))
		return false;

	CallGraphFP &CG = getAnalysis<CallGraphFP>();
	FuncList callees = CG.get_called_functions(ins);
	for (size_t i = 0; i < callees.size(); ++i) {
		if (may_exec_landmark(callees[i]))
			return true;
	}
	return false;
}

bool Exec::must_exec_landmark(const Instruction *ins) const {
	if (is_landmark(ins))
		return true;
	if (!is_call(ins))
		return false;

	CallGraphFP &CG = getAnalysis<CallGraphFP>();

	FuncList callees = CG.get_called_functions(ins);
	bool all_must_exec = true;
	for (size_t i = 0; i < callees.size(); ++i) {
		if (!must_exec_landmark(callees[i])) {
			all_must_exec = false;
			break;
		}
	}
	return all_must_exec;
}

bool Exec::compute_must_exec(const BasicBlock *bb) {
	/**
	 * TODO: Doesn't support recursive function calls very well. 
	 */
	CallGraphFP &CG = getAnalysis<CallGraphFP>();

	for (BasicBlock::const_iterator ins = bb->begin(); ins != bb->end(); ++ins) {
		if (landmarks.count(ins))
			return true;
		if (is_call(ins)) {
			FuncList callees = CG.get_called_functions(ins);
			bool all_must_exec = true;
			for (size_t i = 0; i < callees.size(); ++i) {
				if (!must_exec.count(callees[i])) {
					all_must_exec = false;
					break;
				}
			}
			if (all_must_exec)
				return true;
		}
	}

	return false;
}

bool Exec::compute_must_exec(const Function *f) {
	ConstBBSet sink;
	for (Function::const_iterator bb = f->begin(); bb != f->end(); ++bb) {
		if (compute_must_exec(bb))
			sink.insert(bb);
	}
	
	Reach<BasicBlock> R;
	ConstBBSet visited;
	R.floodfill(f->begin(), sink, visited);
	
	for (Function::const_iterator bb = f->begin(); bb != f->end(); ++bb) {
		/*
		 * If can reach bb's terminator and bb's terminator is a return,
		 * we can pass this function without touching any landmark. 
		 */
		if (visited.count(bb) && !sink.count(bb) && is_ret(bb->getTerminator()))
			return false;
	}
	return true;
}

void Exec::run() {
	traverse_call_graph();
	compute_must_exec();
}

void Exec::print(llvm::raw_ostream &O, const Module *M) const {
}
