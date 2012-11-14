#include "rcs/IDManager.h"
#include "rcs/util.h"
using namespace rcs;

static RegisterPass<IDManager> X("manage-id",
                                 "Find the instruction with a particular ID; "
                                 "Lookup the ID of an instruction",
                                 false, true);

char IDManager::ID = 0;

void IDManager::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

IDManager::IDManager(): ModulePass(ID) {}

bool IDManager::runOnModule(Module &M) {
  IDMapping.clear();
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator B = F->begin(); B != F->end(); ++B) {
      for (BasicBlock::iterator I = B->begin(); I != B->end(); ++I) {
        unsigned InsID = getInstructionID(I);
        if (InsID != INVALID_ID)
          IDMapping[InsID].push_back(I);
      }
    }
  }
  if (size() == 0)
    errs() << "[Warning] No ID information in this program.\n";

  return false;
}

unsigned IDManager::getInstructionID(const Instruction *I) const {
  MDNode *Node = I->getMetadata("ins_id");
  if (!Node)
    return INVALID_ID;
  assert(Node->getNumOperands() == 1);
  ConstantInt *CI = dyn_cast<ConstantInt>(Node->getOperand(0));
  assert(CI);
  return CI->getZExtValue();
}

Instruction *IDManager::getInstruction(unsigned InsID) const {
  InstList Insts = getInstructions(InsID);
  if (Insts.size() == 0 || Insts.size() > 1)
    return NULL;
  else
    return Insts[0];
}

InstList IDManager::getInstructions(unsigned InsID) const {
  return IDMapping.lookup(InsID);
}

void IDManager::print(raw_ostream &O, const Module *M) const {
#if 0
  vector<pair<unsigned, Instruction *> > Entries;
  for (DenseMap<unsigned, Instruction *>::const_iterator I = IDMapping.begin();
       I != IDMapping.end(); ++I) {
    Entries.push_back(*I);
  }
  sort(Entries.begin(), Entries.end());
  for (size_t i = 0; i < Entries.size(); ++i) {
    Instruction *Ins = Entries[i].second;
    BasicBlock *BB = Ins->getParent();
    Function *F = BB->getParent();
    // Print the function name if <ins> is the function entry. 
    if (Ins == F->getEntryBlock().begin()) 
      O << "\nFunction " << F->getName() << ":\n";
    if (Ins == BB->begin())
      O << "\nBB " << F->getName() << "." << BB->getName() << ":\n";
    O << Entries[i].first << ":\t" << *Ins << "\n";
  }
#endif
}
