// Rename values to their value IDs.

#include <string>
#include <sstream>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DerivedTypes.h"
using namespace llvm;

#include "rcs/IDAssigner.h"

namespace rcs {
struct ValueRenaming: public ModulePass {
  static char ID;

  ValueRenaming();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

 private:
  static bool should_rename(const Value *V);
};
}
using namespace rcs;

static RegisterPass<ValueRenaming> X("rename-values",
                                     "Rename values to their value IDs",
                                     false,
                                     false);

void ValueRenaming::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<IDAssigner>();
}

char ValueRenaming::ID = 0;

ValueRenaming::ValueRenaming(): ModulePass(ID) {}

bool ValueRenaming::should_rename(const Value *V) {
  if (isa<Function>(V))
    return false;
  if (isa<InlineAsm>(V))
    return false;
  if (isa<IntegerType>(V->getType()) || isa<PointerType>(V->getType()))
    return true;
  return false;
}

bool ValueRenaming::runOnModule(Module &M) {
  IDAssigner &IDA = getAnalysis<IDAssigner>();

  for (unsigned VID = 0; VID < IDA.getNumValues(); ++VID) {
    Value *V = IDA.getValue(VID); assert(V);
    if (should_rename(V))
      V->setName("");
  }

  for (unsigned VID = 0; VID < IDA.getNumValues(); ++VID) {
    Value *V = IDA.getValue(VID); assert(V);
    if (should_rename(V)) {
      ostringstream OSS;
      OSS << "x" << VID;
      OSS.flush();
      V->setName(OSS.str());
    }
  }

  return true;
}
