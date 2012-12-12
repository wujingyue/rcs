#define DEBUG_TYPE "rcs-cfg"

#include <vector>
using namespace std;

#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/CFG.h"
using namespace llvm;

#include "rcs/FPCallGraph.h"
#include "rcs/util.h"
#include "rcs/ExecOnce.h"
using namespace rcs;

static RegisterPass<ExecOnce> X(
    "exec-once",
    "Identify instructions that can be executed only once",
    false,
    true);

STATISTIC(NumInstructionsNotExecuted, "Number of instructions not executed");
STATISTIC(NumInstructions, "Number of instructions");

char ExecOnce::ID = 0;

ExecOnce::ExecOnce(): ModulePass(ID) {}

void ExecOnce::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<CallGraph>();
  AU.addRequired<FPCallGraph>();
}

void ExecOnce::print(raw_ostream &O, const Module *M) const {
  O << "List of BBs that can be executed only once:\n";
  forallconst(Module, fi, *M) {
    forallconst(Function, bi, *fi) {
      const BasicBlock *bb = bi;
      if (executed_once(const_cast<BasicBlock *>(bb))) {
        O << "\t" << bb->getParent()->getName() << "."
            << bb->getName() << "\n";
      }
    }
  }
  O << "List of reachable functions:\n";
  forallconst(FuncSet, it, reachable_funcs)
      O << "\t" << (*it)->getName() << "\n";
}

bool ExecOnce::runOnModule(Module &M) {
  identify_reachable_funcs(M);
  identify_twice_bbs(M);
  identify_twice_funcs(M);

  NumInstructionsNotExecuted = 0;
  NumInstructions = 0;
  for (Module::iterator f = M.begin(); f != M.end(); ++f) {
    for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
      for (BasicBlock::iterator ins = bb->begin(); ins != bb->end(); ++ins) {
        ++NumInstructions;
        if (not_executed(ins))
          ++NumInstructionsNotExecuted;
      }
    }
  }

  return false;
}

void ExecOnce::identify_starting_funcs(Module &M, FuncSet &starts) {
  starts.clear();
  // Identify reachable recursive functions. 
  FPCallGraph &CG = getAnalysis<FPCallGraph>();
  CallGraph &raw_CG = CG;
  for (scc_iterator<CallGraph *> si = scc_begin(&raw_CG),
       E = scc_end(&raw_CG); si != E; ++si) {
    if (si.hasLoop()) {
      for (size_t i = 0; i < (*si).size(); ++i) {
        Function *f = (*si)[i]->getFunction();
        if (f && !not_executed(f))
          starts.insert(f);
      }
    }
  } // for scc

  // Identify functions that are called by multiple reachable call sites. 
  forallfunc(M, fi) {
    InstList call_sites = CG.getCallSites(fi);
    unsigned n_reachable_call_sites = 0;
    for (size_t j = 0; j < call_sites.size(); ++j) {
      Instruction *call_site = call_sites[j];
      if (!not_executed(call_site))
        ++n_reachable_call_sites;
    }
    if (n_reachable_call_sites > 1)
      starts.insert(fi);
  }

  // Identify functions that are called inside a loop. 
  forall(BBSet, it, twice_bbs) {
    BasicBlock *bb = *it;
    forall(BasicBlock, ii, *bb) {
      if (is_call(ii)) {
        FuncList callees = CG.getCalledFunctions(ii);
        for (size_t j = 0; j < callees.size(); ++j)
          starts.insert(callees[j]);
      }
    }
  }

  // Check <starts>.
  // No starting point can be NULL. 
  forall(FuncSet, it, starts)
      assert(*it);
}

void ExecOnce::identify_twice_funcs(Module &M) {
  // Identify starting functions. 
  FuncSet starts;
  identify_starting_funcs(M, starts);
  // Propagate via the call graph. 
  twice_funcs.clear();
  forall(FuncSet, it, starts)
      propagate_via_cg(*it, twice_funcs);
}

void ExecOnce::identify_reachable_funcs(Module &M) {
  Function *main = M.getFunction("main");
  assert(main && "Cannot find the main function");
  reachable_funcs.clear();
  propagate_via_cg(main, reachable_funcs);
}

void ExecOnce::propagate_via_cg(Function *f, FuncSet &visited) {
  // The call graph contains some external nodes which don't represent
  // any function. 
  if (!f)
    return;
  if (visited.count(f))
    return;
  visited.insert(f);
  CallGraph &CG = getAnalysis<FPCallGraph>();
  // Operator [] does not change the function mapping. 
  CallGraphNode *x = CG[f];
  for (unsigned i = 0; i < x->size(); ++i) {
    CallGraphNode *y = (*x)[i];
    propagate_via_cg(y->getFunction(), visited);
  }
}

void ExecOnce::identify_twice_bbs(Module &M) {
  twice_bbs.clear();
  forallfunc(M, fi) {
    if (fi->isDeclaration())
      continue;
    // GraphTraits<Module::iterator> is not defined. 
    // Have to convert it to Function *. 
    Function *f = fi;
    for (scc_iterator<Function *> si = scc_begin(f), E = scc_end(f);
         si != E; ++si) {
      if (si.hasLoop()) {
        for (size_t i = 0; i < (*si).size(); ++i) {
          BasicBlock *bb = (*si)[i];
          if (!not_executed(bb))
            twice_bbs.insert((*si)[i]);
        }
      }
    } // for scc
  }
}

bool ExecOnce::executed_once(const Instruction *ins) const {
  return executed_once(ins->getParent());
}

bool ExecOnce::executed_once(const BasicBlock *b) const {
  BasicBlock *bb = const_cast<BasicBlock *>(b);
  return !twice_funcs.count(bb->getParent()) && !twice_bbs.count(bb);
}

bool ExecOnce::executed_once(const Function *f) const {
  Function *func = const_cast<Function *>(f);
  return !twice_funcs.count(func);
}

bool ExecOnce::not_executed(const Instruction *ins) const {
  return not_executed(ins->getParent());
}

bool ExecOnce::not_executed(const BasicBlock *bb) const {
  return not_executed(bb->getParent());
}

bool ExecOnce::not_executed(const Function *func) const {
  return !reachable_funcs.count(const_cast<Function *>(func));
}
