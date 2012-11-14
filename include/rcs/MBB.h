#ifndef __MBB_H
#define __MBB_H

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
using namespace llvm;

namespace rcs {
/**
 * An MBB ends with a TerminatorInst or a CallInst. 
 * Note that an InvokeInst is a TerminatorInst. 
 *
 * begin = the first instruction in the MBB. 
 * end = the successor of its last instruction. 
 *
 * e.g. The following BB has two MBBs. 
 * MBB1: %3 = %1 + %2
 *       call foo()
 * MBB2: invoke bar(%3)
 *
 * MBB1.begin() == %3, MBB1.end() == invoke
 * MBB2.begin() == invoke, MBB2.end() == BB.end()
 */
struct MicroBasicBlock: public ilist_node<MicroBasicBlock> {
  typedef BasicBlock::iterator iterator;
  typedef BasicBlock::const_iterator const_iterator;

  BasicBlock* parent;
  iterator b, e;

  MicroBasicBlock(): parent(NULL) {}
  MicroBasicBlock(BasicBlock* p, iterator bb, iterator ee):
      parent(p), b(bb), e(ee)
  {
    assert(p && b != e);
  }

  iterator begin() { return b; }
  iterator end() { return e; }
  const_iterator begin() const { return b; }
  const_iterator end() const { return e; }
  Instruction &front() { return *b; }
  const Instruction &front() const { return *b; }
  Instruction &back() {
    iterator i = e;
    return *(--i);
  }
  const Instruction &back() const {
    const_iterator i = e;
    return *(--i);
  }

  BasicBlock *getParent() { return parent; }
  const BasicBlock *getParent() const { return parent; }

  iterator getFirstNonPHI();
};

struct MicroBasicBlockBuilder: public ModulePass {
  static char ID;

  typedef iplist<MicroBasicBlock> MBBListType;
  typedef DenseMap<BasicBlock*, MBBListType*> mbbmap_t;

  MicroBasicBlockBuilder();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const { 
    AU.setPreservesAll(); 
  }
  virtual bool runOnModule(Module &M);

  MBBListType::iterator begin(BasicBlock *bb);
  MBBListType::iterator end(BasicBlock *bb);
  MBBListType::iterator parent(const Instruction *ins) {
    return parent_mbb.lookup(ins);
  }

 private:
  mbbmap_t mbbmap;
  DenseMap<const Instruction *, MicroBasicBlock *> parent_mbb;
};

typedef MicroBasicBlockBuilder::MBBListType::iterator mbb_iterator;
}

#endif
