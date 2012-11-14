#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"

using namespace llvm;

namespace rcs {
struct MallocReplacer: public BasicBlockPass {
  static char ID;

  MallocReplacer(): BasicBlockPass(ID) {
    MyMalloc = Malloc = NULL;
  }
  virtual bool runOnBasicBlock(BasicBlock &BB);
  virtual bool doInitialization(Module &M);
  virtual bool doInitialization(Function &F) {
    return BasicBlockPass::doInitialization(F);
  }

 private:
  Function *MyMalloc, *Malloc;
};
}

using namespace rcs;

char MallocReplacer::ID = 0;

static RegisterPass<MallocReplacer> X("replace-malloc",
                                      "Replace malloc with my_malloc",
                                      false,
                                      false);

bool MallocReplacer::doInitialization(Module &M) {
  assert(M.getFunction("my_malloc") == NULL);
  Malloc = M.getFunction("malloc");
  if (Malloc) {
    MyMalloc = Function::Create(Malloc->getFunctionType(),
                                GlobalValue::ExternalLinkage,
                                "my_malloc",
                                &M);
  }
  return true;
}

bool MallocReplacer::runOnBasicBlock(BasicBlock &BB) {
  if (Malloc == NULL) {
    // nothing to replace
    return false;
  }
  for (BasicBlock::iterator Ins = BB.begin(); Ins != BB.end(); ++Ins) {
    CallSite CS(Ins);
    if (CS && CS.getCalledFunction() == Malloc) {
      CS.setCalledFunction(MyMalloc);
    }
  }
  return true;
}
