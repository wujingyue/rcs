#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "rcs/ICFG.h"
using namespace rcs;

ICFGNode *ICFG::getOrInsertMBB(const MicroBasicBlock *mbb) {
  ICFGNode *&node = mbb_to_node[mbb];
  if (node)
    return node;
  node = new ICFGNode(const_cast<MicroBasicBlock *>(mbb), this);
  nodes.push_back(node);
  return node;
}

void ICFGNode::print(raw_ostream &O) const {
  if (mbb == NULL)
    O << "<root>";
  else {
    const BasicBlock *bb = mbb->getParent();
    const Function *f = bb->getParent();
    O << f->getName() << "." << bb->getName();
  }
}

void ICFG::addEdge(const MicroBasicBlock *x, const MicroBasicBlock *y) {
  ICFGNode *node_x = getOrInsertMBB(x);
  ICFGNode *node_y = getOrInsertMBB(y);
  node_x->addSuccessor(node_y);
  node_y->addPredecessor(node_x);
}

void ICFG::print(raw_ostream &O) const {
  for (const_iterator x = begin(); x != end(); ++x) {
    for (ICFGNode::const_iterator si = x->succ_begin();
         si != x->succ_end(); ++si) {
      const ICFGNode *y = *si;
      printEdge(O, x, y);
    }
  }
}

void ICFG::printEdge(raw_ostream &O, const ICFGNode *x, const ICFGNode *y) {
  x->print(O);
  O << " ==> ";
  y->print(O);
  O << "\n";
}

ICFGNode &ICFG::front() {
  ICFGNode *root = NULL;
  for (iterator ii = begin(); ii != end(); ++ii) {
    if (ii->pred_begin() == ii->pred_end()) {
      assert(!root);
      root = ii;
    }
  }
  assert(root);
  return *root;
}

const ICFGNode &ICFG::front() const {
  const ICFGNode *root = NULL;
  for (const_iterator ii = begin(); ii != end(); ++ii) {
    if (ii->pred_begin() == ii->pred_end()) {
      assert(!root);
      root = ii;
    }
  }
  assert(root);
  return *root;
}
