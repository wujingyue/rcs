#ifndef __EXEC_H
#define __EXEC_H

/*
 * Usage: 
 * 1. setup_landmarks
 * 2. run
 * 3. may_exec_landmark?
 */

#include "rcs/util.h"
#include "rcs/typedefs.h"

namespace rcs {
struct Exec: public ModulePass {
  static char ID;

  Exec();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(llvm::raw_ostream &O, const Module *M) const;

  void setup_landmarks(const ConstInstSet& landmarks);
  bool is_landmark(const Instruction *ins) const;
  void run();
  bool may_exec_landmark(const Function *f) const;
  bool may_exec_landmark(const Instruction *ins) const;
  bool must_exec_landmark(const Function *f) const;
  bool must_exec_landmark(const BasicBlock *bb) const;
  /**
   * If <ins> is a CallInst/InvokeInst, this function traces into
   * its callees. 
   */
  bool must_exec_landmark(const Instruction *ins) const;

 private:
  void dfs(const Function *f, ConstFuncMapping &parent);
  void traverse_call_graph();
  void print_call_chain(const Function *f);
  void compute_must_exec();
  bool compute_must_exec(const Function *f);
  bool compute_must_exec(const BasicBlock *bb);

  ConstFuncMapping parent; // Used in DFS
  // must_exec[f] == true if function <f> must execute one of the landmarks. 
  ConstFuncSet must_exec;
  ConstInstSet landmarks;
};
}

#endif
