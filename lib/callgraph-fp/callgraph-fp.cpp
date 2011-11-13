#include <set>
#include <cstdio>
#include <fstream>
using namespace std;

#include "llvm/Support/CommandLine.h"
#include "common/callgraph-fp.h"
#include "common/util.h"
using namespace llvm;

#include "bc2bdd/BddAliasAnalysis.h"
using namespace repair;

static RegisterPass<CallGraphFP> X("callgraph-fp",
		"Call graph that recognizes function pointers",
		false, true); // is analysis

static cl::opt<string> ExtraCallEdgesFile("extra-call-edges",
		cl::desc("The file which contains the extra call edges that need be added "
		"to the call graph"));

char CallGraphFP::ID = 0;

void CallGraphFP::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<BddAliasAnalysis>();
	ModulePass::getAnalysisUsage(AU);
}

void CallGraphFP::destroy() {
	// <calls_extern_node> is not in the function map, delete it explicitly. 
	delete calls_extern_node;
	calls_extern_node = NULL;
	CallGraph::destroy();
}

void CallGraphFP::add_call_edge(const CallSite &site, Function *callee) {
	Instruction *ins = site.getInstruction();
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
		set<const Function *> &all_funcs) {
	BddAliasAnalysis &BAA = getAnalysis<BddAliasAnalysis>();

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
				// Ask BAA which functions <fp> may point to. 
				set<const Value *> pointees;
				BAA.pointsTo(target, 0, &all_funcs, pointees);
#if 0
				if (pointees.size() == 0) {
					errs() << "[Warning] the function pointer below points to "
						"nothing\n" << *target << "\n";
				}
#endif
				forall(set<const Value *>, it, pointees) {
					add_call_edge(
							cs, const_cast<Function *>(dyn_cast<Function>(*it)));
				}
			}
		}
	} else {
		Value *fp = cs.getCalledValue();
		assert(fp && "Cannot find the function pointer");
		// Ask BAA which functions <fp> may point to. 
		set<const Value *> pointees;
		BAA.pointsTo(fp, 0, &all_funcs, pointees);
#if 0
		if (pointees.size() == 0) {
			errs() << "[Warning] the function pointer below points to "
				"nothing\n" << *fp << "\n";
		}
#endif
		forall(set<const Value *>, it, pointees)
			add_call_edge(cs, const_cast<Function *>(dyn_cast<Function>(*it)));
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
	set<const Function *> all_funcs;
	forallfunc(M, f) {
		if (!f->isDeclaration())
			all_funcs.insert(f);
	}

	/* Get root (main function) */
	unsigned n_mains = 0;
	forallfunc(M, fi) {
		if (!fi->hasLocalLinkage() && fi->getNameStr() == "main") {
			n_mains++;
			root = getOrInsertFunction(fi);
		}
	}
	// No root if no main function or more than one main functions. 
	if (n_mains != 1)
		root = extern_calling_node;
	
	// Connect <extern_calling_node>
	if (root != extern_calling_node) {
		extern_calling_node->addCalledFunction(CallSite(), root);
	} else {
		forallfunc(M, fi) {
			if (!fi->hasLocalLinkage()) {
				extern_calling_node->addCalledFunction(
						CallSite(), getOrInsertFunction(fi));
			}
		}
	}
	
	// Connect <calls_extern_node>.
	forallfunc(M, fi) {
		if (fi->isDeclaration()) {
			getOrInsertFunction(fi)->addCalledFunction(
					CallSite(), calls_extern_node);
		}
	}

	// Build the call graph. 
	called_funcs.clear();
	call_sites.clear();
	forallinst(M, ins) {
		CallSite cs = CallSite::get(ins);
		if (cs.getInstruction())
			process_call_site(cs, all_funcs);
	}

	add_extra_call_edges(M);

	// Simplify the call graph. 
	simplify_call_graph();

	return false;
}

void CallGraphFP::add_extra_call_edges(Module &M) {
	if (ExtraCallEdgesFile == "")
		return;

	ifstream fin(ExtraCallEdgesFile.c_str());
	assert(fin && "Cannot open the extra call edges file");
	string caller_name, callee_name;
	while (fin >> caller_name >> callee_name) {
		Function *caller = M.getFunction(caller_name);
		Function *callee = M.getFunction(callee_name);
		if (!caller || !callee) {
			errs() << "[Warning] Failed to add call edge " << caller_name <<
				" => " << callee_name << "\n";
			continue;
		}
		forall(Function, bb, *caller) {
			forall(BasicBlock, ins, *bb) {
				CallSite cs = CallSite::get(ins);
				if (cs.getInstruction() && !cs.getCalledFunction()) {
					Value *fp = cs.getCalledValue();
					assert(fp);
					if (fp->getType() == callee->getType())
						add_call_edge(cs, callee);
					else if (caller_name == "apr_table_do" &&
							callee_name == "form_header_field")
						add_call_edge(cs, callee);
				}
			}
		}
	}
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
