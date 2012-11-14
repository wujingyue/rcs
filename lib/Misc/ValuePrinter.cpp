#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace rcs {
struct ValuePrinter: public ModulePass {
  static char ID;
  ValuePrinter(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &O, const Module *M) const;
};
}

using namespace rcs;

static RegisterPass<ValuePrinter> X("print-values",
                                    "Print values",
                                    false, // Is CFG Only?
                                    true); // Is Analysis?

char ValuePrinter::ID = 0;

void ValuePrinter::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool ValuePrinter::runOnModule(Module &M) {
  return false;
}

void ValuePrinter::print(raw_ostream &O, const Module *M) const {
  errs() << "# of functions = " << M->size() << "\n";
  unsigned NumFunctionsPrinted = 0;
  for (Module::const_iterator F = M->begin(); F != M->end(); ++F) {
    errs() << "Function " << NumFunctionsPrinted << ": "
        << F->getName() << "\n";
    for (Function::const_iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::const_iterator Ins = BB->begin(); Ins != BB->end();
           ++Ins) {
        Ins->print(O);
      }
    }
    ++NumFunctionsPrinted;
  }
#if 0
  M->print(O, NULL);
#endif
}
