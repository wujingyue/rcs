#include "llvm/Support/CFG.h"

#include "rcs/IntraReach.h"
#include "rcs/util.h"

using namespace llvm;
using namespace rcs;

static RegisterPass<IntraReach> X(
    "intra-reach",
    "Intra-procedural reachability analysis",
    false,
    true);

char IntraReach::ID = 0;

IntraReach::IntraReach(): FunctionPass(ID) {}

void IntraReach::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool IntraReach::runOnFunction(Function &F) {
  return false;
}

bool IntraReach::reachable(const BasicBlock *x, const BasicBlock *y) const {
  ConstBBSet visited;
  ConstBBSet sink;
  sink.insert(y);
  floodfill(x, sink, visited);
  return visited.count(y);
}

void IntraReach::floodfill_r(
    const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const {
  if (visited.count(x))
    return;
  visited.insert(x);
  if (sink.count(x))
    return;
  for (const_pred_iterator pi = pred_begin(x), E = pred_end(x);
       pi != E; ++pi) {
    floodfill_r(*pi, sink, visited);
  }
}

void IntraReach::floodfill(
    const BasicBlock *x, const ConstBBSet &sink, ConstBBSet &visited) const {
  if (visited.count(x))
    return;
  visited.insert(x);
  if (sink.count(x))
    return;
  for (succ_const_iterator si = succ_begin(x), E = succ_end(x);
       si != E; ++si) {
    floodfill(*si, sink, visited);
  }
}
