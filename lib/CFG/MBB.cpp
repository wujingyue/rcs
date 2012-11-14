/**
 * TODO: Put it into folder <cfg>. <cfg> now requires <bc2bdd>, and we
 * don't want <mbb> heavy-weighted. 
 */

#include "rcs/MBB.h"
#include "rcs/util.h"
using namespace rcs;

static RegisterPass<MicroBasicBlockBuilder> X("mbbb",
                                              "micro basic block builder",
                                              false,
                                              false);

MicroBasicBlockBuilder::MicroBasicBlockBuilder(): ModulePass(ID) {}

MicroBasicBlock::iterator MicroBasicBlock::getFirstNonPHI() {
  if (b == parent->begin())
    return parent->getFirstNonPHI();
  return b;
}

bool MicroBasicBlockBuilder::runOnModule(Module &M) {
  parent_mbb.clear();
  mbbmap.clear();

  forallbb(M, bb) {
    MBBListType *mbblist = new MBBListType;
    for (BasicBlock::iterator ib = bb->begin(); ib != bb->end(); ) {
      BasicBlock::iterator ie = ib;
      while (bb->getTerminator() != ie && !is_non_intrinsic_call(ie))
        ++ie;
      // <ie> points to the last instruction of the MBB. 
      assert(ie != bb->end());
      ++ie;
      // <ie> points to the successor of the MBB. 
      MicroBasicBlock *mbb = new MicroBasicBlock(bb, ib, ie);
      mbblist->push_back(mbb);
      // Reverse mapping from Instruction to its containing MBB. 
      for (BasicBlock::iterator ii = ib; ii != ie; ++ii)
        parent_mbb[ii] = mbb;
      ib = ie;
    }
    mbbmap[bb] = mbblist;
  }

  return false;
}

mbb_iterator MicroBasicBlockBuilder::begin(BasicBlock *bb) {
  assert(exist(bb, mbbmap) && "not a basic block");
  return mbbmap[bb]->begin();
}

mbb_iterator MicroBasicBlockBuilder::end(BasicBlock *bb) {
  assert(exist(bb, mbbmap) && "not a basic block");
  return mbbmap[bb]->end();
}

char MicroBasicBlockBuilder::ID = 0;
