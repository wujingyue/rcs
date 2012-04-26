/**
 * Author: Jingyue
 *
 * A call-graph builder considering function pointers. 
 * The targets of function pointers are identified by alias analysis. 
 * Users may specify which alias analysis she wants to run this pass with. 
 */

#include <cstdio>
#include <fstream>
using namespace std;

#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "common/InitializePasses.h"
#include "bc2bdd/InitializePasses.h"
using namespace llvm;

#include "common/callgraph-fp.h"
#include "common/fp-collector.h"
#include "common/util.h"
using namespace rcs;

INITIALIZE_PASS_BEGIN(CallGraphFP, "callgraph-fp",
		"Call graph that recognizes function pointers", false, true)
INITIALIZE_PASS_DEPENDENCY(FPCollector)
INITIALIZE_AG_DEPENDENCY(AliasAnalysis)
INITIALIZE_PASS_END(CallGraphFP, "callgraph-fp",
		"Call graph that recognizes function pointers", false, true)

char CallGraphFP::ID = 0;

void CallGraphFP::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<AliasAnalysis>();
	AU.addRequired<FPCollector>();
}

CallGraphFP::CallGraphFP():
	ModulePass(ID), root(NULL), extern_calling_node(NULL),
	calls_extern_node(NULL)
{
	initializeCallGraphFPPass(*PassRegistry::getPassRegistry());
}

void CallGraphFP::destroy() {
	// <calls_extern_node> is not in the function map, delete it explicitly. 
	delete calls_extern_node;
	calls_extern_node = NULL;
	CallGraph::destroy();
}

void CallGraphFP::add_call_edge(const CallSite &site, Function *callee) {
	Instruction *ins = site.getInstruction();
	assert(ins);
	called_funcs[ins].push_back(callee);
	call_sites[callee].push_back(ins);
	// Update CallGraph as well. 
	CallGraphNode *node = getOrInsertFunction(ins->getParent()->getParent());
	node->addCalledFunction(site, getOrInsertFunction(callee));
}

template <class T>
void CallGraphFP::make_unique(vector<T> &v) {
	sort(v.begin(), v.end());
	v.resize(unique(v.begin(), v.end()) - v.begin());
}

FuncList CallGraphFP::get_called_functions(
		const Instruction *ins) const {
	SiteFuncMapping::const_iterator it = called_funcs.find(ins);
	if (it == called_funcs.end())
		return FuncList();
	return it->second;
}

InstList CallGraphFP::get_call_sites(
		const Function *f) const {
	FuncSiteMapping::const_iterator it = call_sites.find(f);
	if (it == call_sites.end())
		return InstList();
	return it->second;
}

void CallGraphFP::process_call_site(const CallSite &cs,
		const FuncSet &all_funcs) {
	AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

	if (Function *callee = cs.getCalledFunction()) {
		add_call_edge(cs, callee);
		const Instruction *ii = cs.getInstruction();
		if (is_pthread_create(ii)) {
			// Add edge: ii => the thread function
			Value *target = get_pthread_create_callee(ii);
			if (Function *thr_func = dyn_cast<Function>(target)) {
				// pthread_create with a known function
				add_call_edge(cs, thr_func);
			} else {
				// Ask AA which functions <target> may point to. 
				for (FuncSet::const_iterator it = all_funcs.begin();
						it != all_funcs.end(); ++it) {
					if (AA.alias(target, *it))
						add_call_edge(cs, *it);
				}
			}
		}
	} else {
		Value *fp = cs.getCalledValue();
		assert(fp && "Cannot find the function pointer");
		// Ask AA which functions <fp> may point to. 
		for (FuncSet::const_iterator it = all_funcs.begin();
				it != all_funcs.end(); ++it) {
			if (AA.alias(fp, *it))
				add_call_edge(cs, *it);
		}
	}
}

bool CallGraphFP::runOnModule(Module &M) {
	// Initialize super class CallGraph.
	CallGraph::initialize(M);

	// Use getOrInsertFunction(NULL) so that
	// extern_calling_node->getFunction() returns NULL. 
	extern_calling_node = getOrInsertFunction(NULL);
	calls_extern_node = new CallGraphNode(NULL);

	// Every function need to have a corresponding CallGraphNode. 
	forallfunc(M, fi)
		getOrInsertFunction(fi);

	/*
	 * Get the set of all defined functions. 
	 * Will be used as a candidate set for point-to analysis. 
	 * FIXME: Currently we have to skip external functions, otherwise
	 * bc2bdd would fail. Don't ask me why. 
	 */
	FuncSet all_funcs;
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		if (!f->isDeclaration())
			all_funcs.insert(f);
	}

	/* Get root (main function) */
	unsigned n_mains = 0;
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		if (!f->hasLocalLinkage() && f->getNameStr() == "main") {
			n_mains++;
			root = getOrInsertFunction(f);
		}
	}
	// No root if no main function or more than one main functions. 
	if (n_mains != 1)
		root = extern_calling_node;
	
	// Connect <extern_calling_node>
	if (root != extern_calling_node) {
		extern_calling_node->addCalledFunction(CallSite(), root);
	} else {
		for (Module::iterator f = M.begin(); f != M.end(); ++f) {
			if (!f->hasLocalLinkage()) {
				extern_calling_node->addCalledFunction(
						CallSite(), getOrInsertFunction(f));
			}
		}
	}
	
	// Connect <calls_extern_node>.
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		if (f->isDeclaration()) {
			getOrInsertFunction(f)->addCalledFunction(
					CallSite(), calls_extern_node);
		}
	}

	// Build the call graph. 
	called_funcs.clear();
	call_sites.clear();
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
			for (BasicBlock::iterator ins = bb->begin(); ins != bb->end(); ++ins) {
				CallSite cs(ins);
				if (cs.getInstruction())
					process_call_site(cs, all_funcs);
			}
		}
	}

	// Add extra call edges via function pointers. 
	FPCollector &FPC = getAnalysis<FPCollector>();
	for (unsigned i = 0; i < FPC.get_num_call_edges(); ++i) {
		pair<Instruction *, Function *> edge = FPC.get_call_edge(i);
		add_call_edge(CallSite(edge.first), edge.second);
	}

	// Simplify the call graph. 
	simplify_call_graph();

	return false;
}

void CallGraphFP::simplify_call_graph() {
	// Remove duplicated items in each vector. 
	forall(SiteFuncMapping, it, called_funcs)
		make_unique(it->second);
	forall(FuncSiteMapping, it, call_sites)
		make_unique(it->second);
}

void CallGraphFP::print(llvm::raw_ostream &O, const Module *M) const {
	O << "Caller - Callee:\n";
	for (Module::const_iterator fi = M->begin(); fi != M->end(); ++fi) {
		// All called functions inside <fi>. 
		FuncList all_callees;
		for (Function::const_iterator bi = fi->begin(); bi != fi->end(); ++bi) {
			for (BasicBlock::const_iterator ii = bi->begin();
					ii != bi->end(); ++ii) {
				if (is_call(ii)) {
					const FuncList &called_funcs = get_called_functions(ii);
					for (FuncList::const_iterator it = called_funcs.begin();
							it != called_funcs.end(); ++it)
						all_callees.push_back(*it);
				}
			}
		}
		sort(all_callees.begin(), all_callees.end());
		all_callees.resize(unique(all_callees.begin(), all_callees.end())
				- all_callees.begin());
		if (!all_callees.empty()) {
			O << "\t" << fi->getNameStr() << " calls:\n";
			forall(FuncList, it, all_callees)
				O << "\t\t" << (*it)->getNameStr() << "\n";
		}
	}
	O << "Callee - Caller:\n";
	for (Module::const_iterator fi = M->begin(); fi != M->end(); ++fi) {
		// All calling functions to <fi>.
		const InstList &call_sites = get_call_sites(fi);
		FuncList all_callers;
		for (InstList::const_iterator it = call_sites.begin();
				it != call_sites.end(); ++it)
			all_callers.push_back((*it)->getParent()->getParent());
		sort(all_callers.begin(), all_callers.end());
		all_callers.resize(unique(all_callers.begin(), all_callers.end())
				- all_callers.begin());
		if (!all_callers.empty()) {
			O << "\t" << fi->getNameStr() << " is called by:\n";
			forall(FuncList, it, all_callers)
				O << "\t\t" << (*it)->getNameStr() << "\n";
		}
	}
}
