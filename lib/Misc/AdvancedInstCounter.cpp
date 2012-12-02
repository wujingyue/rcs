#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace rcs {
struct AdvancedInstCounter: public ModulePass {
  static char ID;
  AdvancedInstCounter(): ModulePass(ID) {}
  virtual bool runOnModule(Module &M);
};
}

using namespace rcs;

static RegisterPass<AdvancedInstCounter> X(
    "count-insts",
    "Count various types of instructions",
    false, // Is CFG Only?
    true); // Is Analysis?

char AdvancedInstCounter::ID = 0;

bool AdvancedInstCounter::runOnModule(Module &M) {
  unsigned NumIndirectCalls = 0;
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator B = F->begin(); B != F->end(); ++B) {
      for (BasicBlock::iterator I = B->begin(); I != B->end(); ++I) {
        CallSite CS(I);
        if (CS && CS.getCalledFunction() == NULL) {
          ++NumIndirectCalls;
        }
      }
    }
  }
  errs() << "# of indirect calls = " << NumIndirectCalls << "\n";
  return false;
}
