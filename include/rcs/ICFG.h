// Similar to llvm::CallGraph

#ifndef __ICFG_H
#define __ICFG_H

#include <vector>
using namespace std;

#include "llvm/ADT/GraphTraits.h"
using namespace llvm;

#include "rcs/MBB.h"
#include "rcs/typedefs.h"
using namespace rcs;

namespace rcs {
struct ICFG;

struct ICFGNode: public ilist_node<ICFGNode> {

  typedef vector<ICFGNode *>::iterator iterator;
  typedef vector<ICFGNode *>::const_iterator const_iterator;

  ICFGNode(): mbb(NULL), parent(NULL) {}
  ICFGNode(MicroBasicBlock *m, ICFG *p): mbb(m), parent(p) {}

  iterator succ_begin() { return succs.begin(); }
  iterator succ_end() { return succs.end(); }
  const_iterator succ_begin() const { return succs.begin(); }
  const_iterator succ_end() const { return succs.end(); }
  iterator pred_begin() { return preds.begin(); }
  iterator pred_end() { return preds.end(); }
  const_iterator pred_begin() const { return preds.begin(); }
  const_iterator pred_end() const { return preds.end(); }

  MicroBasicBlock *getMBB() const { return mbb; }
  const ICFG *getParent() const { return parent; }
  ICFG *getParent() { return parent; }
  unsigned size() const { return (unsigned)succs.size(); }
  void addSuccessor(ICFGNode *succ) { succs.push_back(succ); }
  void addPredecessor(ICFGNode *pred) { preds.push_back(pred); }
  void print(raw_ostream &O) const;

 private:
  MicroBasicBlock *mbb;

  // Need maintain successors and predecessors in order to implement
  // GraphTraits<Inverse<ICFGNode *> >
  vector<ICFGNode *> succs, preds;
  ICFG *parent;
};

struct ICFG {
  typedef DenseMap<const MicroBasicBlock *, ICFGNode *> MBBToNode;
  typedef iplist<ICFGNode>::iterator iterator;
  typedef iplist<ICFGNode>::const_iterator const_iterator;

  ICFG() {}

  // Returns NULL if <mbb> is not in the ICFG. 
  const ICFGNode *operator[](const MicroBasicBlock *mbb) const {
    return mbb_to_node.lookup(mbb);
  }
  ICFGNode *operator[](const MicroBasicBlock *mbb) {
    return mbb_to_node.lookup(mbb);
  }

  iterator begin() { return nodes.begin(); }
  iterator end() { return nodes.end(); }
  const_iterator begin() const { return nodes.begin(); }
  const_iterator end() const { return nodes.end(); }
  ICFGNode &front();
  const ICFGNode &front() const;
  size_t size() const {
    assert(mbb_to_node.size() == nodes.size());
    return nodes.size();
  }

  /**
   * This must be the only interface to create a new node. 
   * <addEdge> calls <getOrInsertMBB>, so it's fine. 
   */
  ICFGNode *getOrInsertMBB(const MicroBasicBlock *mbb);
  void addEdge(const MicroBasicBlock *x, const MicroBasicBlock *y);

  // Print functions. 
  static void printEdge(raw_ostream &O, const ICFGNode *x, const ICFGNode *y);
  void print(raw_ostream &O) const;

 private:
  MBBToNode mbb_to_node;
  iplist<ICFGNode> nodes;
};
}

namespace llvm {
template<>
struct GraphTraits<ICFGNode *> {
  typedef ICFGNode NodeType;
  typedef ICFGNode::iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *node) { return node; }
  static ChildIteratorType child_begin(NodeType *x) { return x->succ_begin(); }
  static ChildIteratorType child_end(NodeType *x) { return x->succ_end(); }
};

template<>
struct GraphTraits<const ICFGNode *> {
  typedef const ICFGNode NodeType;
  typedef ICFGNode::const_iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *node) { return node; }
  static ChildIteratorType child_begin(NodeType *x) { return x->succ_begin(); }
  static ChildIteratorType child_end(NodeType *x) { return x->succ_end(); }
};

template<>
struct GraphTraits<Inverse<ICFGNode *> > {
  typedef ICFGNode NodeType;
  typedef ICFGNode::iterator ChildIteratorType;

  static NodeType *getEntryNode(Inverse<NodeType *> node) {
    return node.Graph;
  }
  static ChildIteratorType child_begin(NodeType *x) { return x->pred_begin(); }
  static ChildIteratorType child_end(NodeType *x) { return x->pred_end(); }
};

template<>
struct GraphTraits<Inverse<const ICFGNode *> > {
  typedef const ICFGNode NodeType;
  typedef ICFGNode::const_iterator ChildIteratorType;

  static NodeType *getEntryNode(Inverse<NodeType *> node) {
    return node.Graph;
  }
  static ChildIteratorType child_begin(NodeType *x) { return x->pred_begin(); }
  static ChildIteratorType child_end(NodeType *x) { return x->pred_end(); }
};

template<>
struct GraphTraits<ICFG *>: public GraphTraits<ICFGNode *> {
  typedef ICFG::iterator nodes_iterator;
  static nodes_iterator nodes_begin(ICFG *icfg) { return icfg->begin(); }
  static nodes_iterator nodes_end(ICFG *icfg) { return icfg->end(); }
};

template<>
struct GraphTraits<const ICFG *>: public GraphTraits<const ICFGNode *> {
  typedef ICFG::const_iterator nodes_iterator;
  static nodes_iterator nodes_begin(const ICFG *icfg) { return icfg->begin(); }
  static nodes_iterator nodes_end(const ICFG *icfg) { return icfg->end(); }
};
}

#endif
