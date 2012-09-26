// Author: Jingyue

#include <string>

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "rcs/IDAssigner.h"

using namespace std;
using namespace llvm;
using namespace rcs;

namespace rcs {
struct IDLookUp: public ModulePass {
  static char ID;

  IDLookUp(): ModulePass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
};
}

static RegisterPass<IDLookUp> X(
    "lookup-id",
    "Look up the ID of a named value",
    false,
    true);

static cl::opt<string> TheFunctionName(
    "func-name",
    cl::desc("Function name"));
static cl::opt<string> TheValueName(
    "value-name",
    cl::desc("Value name"));

char IDLookUp::ID = 0;

void IDLookUp::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<IDAssigner>();
  AU.setPreservesAll();
}

bool IDLookUp::runOnModule(Module &M) {
  assert(TheValueName != "");

  Value *V = NULL;
  if (TheFunctionName == "") {
    // Look up a global value.
    V = M.getNamedValue(TheValueName);
  } else {
    // Look up a local value inside <TheFunctionName>.
    if (Function *F = M.getFunction(TheFunctionName)) {
      for (Function::arg_iterator AI = F->arg_begin();
           AI != F->arg_end(); ++AI) {
        if (AI->getName() == TheValueName) {
          V = AI;
          break;
        }
      }
      if (V == NULL) {
        for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
          for (BasicBlock::iterator Ins = BB->begin();
               Ins != BB->end(); ++Ins) {
            if (Ins->getName() == TheValueName) {
              V = Ins;
              break;
            }
          }
          if (V != NULL)
            break;
        }
      }
    }
  }

  if (V == NULL) {
    errs() << "Not found\n";
  } else {
    IDAssigner &IDA = getAnalysis<IDAssigner>();
    errs() << "Value ID = " << IDA.getValueID(V) << "\n";
  }

  return false;
}
