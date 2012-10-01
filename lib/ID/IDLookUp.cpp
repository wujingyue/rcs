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

 private:
  void lookUpValueByID(unsigned TheValueID);
  Value *lookUpValueByName(Module &M,
                           const string &TheFunctionName,
                           const string &TheValueName);
  void lookUpIDByName(Module &M,
                      const string &TheFunctionName,
                      const string &TheValueName);
};
}

static RegisterPass<IDLookUp> X(
    "lookup-id",
    "Look up the ID of a named value or vice versa",
    false,
    true);

static cl::opt<string> TheFunctionName(
    "func-name",
    cl::desc("Function name"));
static cl::opt<string> TheValueName(
    "value-name",
    cl::desc("Value name"));
static cl::opt<unsigned> TheValueID(
    "value-id",
    cl::init(IDAssigner::INVALID_ID),
    cl::desc("Value ID"));

char IDLookUp::ID = 0;

void IDLookUp::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<IDAssigner>();
  AU.setPreservesAll();
}

bool IDLookUp::runOnModule(Module &M) {
  assert((TheValueName == "" || TheValueID == IDAssigner::INVALID_ID) &&
         "Cannot specify both value-name and value-id");
  if (TheValueID != IDAssigner::INVALID_ID) {
    lookUpValueByID(TheValueID);
  } else {
    lookUpIDByName(M, TheFunctionName, TheValueName);
  }
  return false;
}

void IDLookUp::lookUpValueByID(unsigned TheValueID) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();
  if (Value *V = IDA.getValue(TheValueID)) {
    IDA.printValue(errs(), V);
    errs() << "\n";
  } else {
    errs() << "Not found\n";
  }
}

void IDLookUp::lookUpIDByName(Module &M,
                              const string &TheFunctionName,
                              const string &TheValueName) {
  assert(TheValueName != "");

  if (Value *V = lookUpValueByName(M, TheFunctionName, TheValueName)) {
    IDAssigner &IDA = getAnalysis<IDAssigner>();
    errs() << "Value ID = " << IDA.getValueID(V) << "\n";
  } else {
    errs() << "Not found\n";
  }
}

Value *IDLookUp::lookUpValueByName(Module &M,
                                   const string &TheFunctionName,
                                   const string &TheValueName) {
  if (TheFunctionName == "") {
    // Look up a global value.
    return M.getNamedValue(TheValueName);
  }

  // Look up a local value inside <TheFunctionName>.
  if (Function *F = M.getFunction(TheFunctionName)) {
    for (Function::arg_iterator AI = F->arg_begin();
         AI != F->arg_end(); ++AI) {
      if (AI->getName() == TheValueName) {
        return AI;
      }
    }
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator Ins = BB->begin();
           Ins != BB->end(); ++Ins) {
        if (Ins->getName() == TheValueName) {
          return Ins;
        }
      }
    }
  }

  return NULL;
}
