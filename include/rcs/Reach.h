// TODO: Make methods static?
// Move complicated logic into a *-inl.h file.

#ifndef __REACH_H
#define __REACH_H

#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/DenseSet.h"
using namespace llvm;

#include "rcs/util.h"
using namespace rcs;

namespace rcs {
template <class Node>
struct Reach {
  typedef DenseSet<const Node *> ConstNodeSet;

  void floodfill(const Node *x, const ConstNodeSet &sink,
                 ConstNodeSet &visited) const {
    if (visited.count(x))
      return;
    visited.insert(x);
    if (sink.count(x))
      return;
    for (typename GraphTraits<const Node *>::ChildIteratorType
         si = GraphTraits<const Node *>::child_begin(x);
         si != GraphTraits<const Node *>::child_end(x); ++si) {
      floodfill(*si, sink, visited);
    }
  }

  /**
   * <sink> may be visited as well. 
   */
  void floodfill(const ConstNodeSet &src, const ConstNodeSet &sink,
                 ConstNodeSet &visited) const {
    forallconst(typename ConstNodeSet, it, src)
        floodfill(*it, sink, visited);
  }

  /**
   * <sink> may be visited as well. 
   */
  void floodfill_r(const Node *x, const ConstNodeSet &sink,
                   ConstNodeSet &visited) const {
    if (visited.count(x))
      return;
    visited.insert(x);
    if (sink.count(x))
      return;
    for (typename GraphTraits<Inverse<const Node *> >::ChildIteratorType
         si = GraphTraits<Inverse<const Node *> >::child_begin(x);
         si != GraphTraits<Inverse<const Node *> >::child_end(x); ++si) {
      floodfill_r(*si, sink, visited);
    }
  }

  void floodfill_r(const ConstNodeSet &src, const ConstNodeSet &sink,
                   ConstNodeSet &visited) const {
    forallconst(typename ConstNodeSet, it, src)
        floodfill_r(*it, sink, visited);
  }

  bool reachable(const Node *x, const Node *y) const {
    ConstNodeSet visited, sink;
    sink.insert(y);
    floodfill(x, sink, visited);
    return visited.count(y);
  }
};
}

#endif
