// Decide whether an instruction can be executed only once (or never).
// TODO: Merge with Exec.

#ifndef __EXEC_ONCE_H
#define __EXEC_ONCE_H

#include "llvm/Module.h"
#include "llvm/Pass.h"
using namespace llvm;

#include "rcs/typedefs.h"
using namespace rcs;

namespace rcs {
struct ExecOnce: public ModulePass {
  static char ID;

  ExecOnce();
  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void print(raw_ostream &O, const Module *M) const;

  /* Executed <= once? */
  bool executed_once(const Instruction *ins) const;
  bool executed_once(const BasicBlock *bb) const;
  bool executed_once(const Function *func) const;

  /* Not executed at all */
  bool not_executed(const Instruction *ins) const;
  bool not_executed(const BasicBlock *bb) const;
  bool not_executed(const Function *func) const;

 private:
  // Used in <identify_twice_funcs>. 
  void identify_starting_funcs(Module &M, FuncSet &starts);
  // Identify all functions that can be executed more than once. 
  void identify_twice_funcs(Module &M);
  // Identify all BBs in reachable loops. 
  // Note that this set isn't equivalent to all the BBs that can be
  // executed more than once. 
  void identify_twice_bbs(Module &M);
  // Identify all reachable functions.
  void identify_reachable_funcs(Module &M);
  // DFS from a starting function via the call graph. 
  void propagate_via_cg(Function *f, FuncSet &visited);

  // Results of function <identify_twice_funcs>.
  FuncSet twice_funcs;
  // Results of function <identify_reachable_funcs>.
  FuncSet reachable_funcs;
  // Results of function <identify_twice_bbs>.
  BBSet twice_bbs;
};
}

#endif
