#ifndef __IDENTIFY_THREAD_FUNCS_H
#define __IDENTIFY_THREAD_FUNCS_H

#include "llvm/Function.h"
#include "llvm/Pass.h"
using namespace llvm;

#include "rcs/typedefs.h"
using namespace rcs;

namespace rcs {
struct IdentifyThreadFuncs: public ModulePass {
  static char ID;

  IdentifyThreadFuncs();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &O, const Module *M) const;
  bool isThreadFunction(const Function &F) const;

 private:
  ConstFuncSet ThreadFuncs;
};
}

#endif
