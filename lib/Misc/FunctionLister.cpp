#include <vector>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace rcs {
struct FunctionLister: public ModulePass {
  static char ID;

  FunctionLister();
  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};
}
using namespace rcs;

static RegisterPass<FunctionLister> X("list-functions",
                                      "List all functions in a module",
                                      false,
                                      true);

char FunctionLister::ID = 0;

void FunctionLister::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

FunctionLister::FunctionLister(): ModulePass(ID) {}

bool FunctionLister::runOnModule(Module &M) {
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
