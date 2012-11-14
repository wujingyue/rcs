/**
 * DEPRECATED
 *
 * How to install:
 * ./configure [-v|--verbose]
 * make
 *
 * This LLVM pass performs a reachability analysis on a module. It can also run
 * standalonely with a specified input. The input file, which is specified by an
 * option, contains the start instruction ID, and the instruction IDs in the cut.
 * The pass floodfills from <start> without touching any instruction in the cut,
 * and finally prints the instructions visited. 
 *
 * Command line: opt ... -reach -input <file name> < XXX.bc > /dev/null
 *
 * Input format:
 * <start ID> // start
 * <ID1> <ID2> ... <IDn> // cut
 *
 * Output:
 * <ID1> <ID2> ... <IDk>
 */

#ifndef __REACH_H
#define __REACH_H

#include <vector>
using namespace std;

#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
using namespace llvm;

namespace rcs {
struct Reachability: public ModulePass {
  typedef DenseMap<BasicBlock *, vector<BasicBlock *> > ParPostdomGraph;
  typedef DenseMap<BasicBlock *, int> BBSCCMapping;
  typedef vector<vector<BasicBlock *> > SCCBBMapping;
  typedef DenseSet<Instruction *> InstSet;
  typedef pair<Instruction *, Instruction *> InstPair;
  typedef DenseSet<InstPair> InstPairSet;

  static char ID;

  Reachability();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  // Returns whether a thread may go from Instruction <start> to
  // Instruction <end>. 
  // This routine calls floodfill, therefore it runs as slow as floodfill
  // even though floodfill 
  bool reachable(
      Module *module,
      Instruction *start,
      Instruction *end,
      bool backwards = false);
  /*
   * Floodfills from <start> without touching instructions in <cut>. 
   * The result will be stored in <result>. 
   * This routine takes linear time. 
   *
   * The cut consists of the entries of every instructions in <cut>. 
   */
  void floodfill(
      Module *module,
      Instruction *start,
      const InstSet &cut,
      InstSet &visited,
      InstPairSet &visited_edges,
      bool backwards = false);
  /* 
   * Calculate par_postdomed for every BBs in the module,
   * and further calculate uncrossable for every functions. 
   *
   * par_postdomed[BB] = true iff every path from BB to an exit of 
   * BB's parent includes an instruction in <cut>. In another word, you cannot
   * exit BB's parent (which is a function) without touching any instruction
   * in <cut>. 
   *
   * uncrossable[F] = true iff par_postdomed[F->entry()] == true. 
   *
   * par_postdomed is calculated recursively. 
   *
   * par_postdomed[BB] = OR_{each call instruction C in BB}(
   *   par_postdomed[C's target's entry block]) OR
   *   AND_{each successor S of BB}(par_postdomed(S))
   *
   * Note that if a call instruction has multiple target functions (e.g.
   * indirect calls), then
   * par_postdomed[C's target's entry block] = AND_{each possible target T}
   *   (par_postdomed[T's entry block])
   *
   * We've built the partial post-dominance graph using
   * <build_par_postdom_graph>. We've also topologically sorted the 
   * basic blocks according to the order they will be referenced by
   * the above formula (Basic blocks in one SCC are also identified). 
   * Therefore, we only need to go backwards in the topological
   * order, and calculate <par_postdomed>. In each SCC, we iteratively
   * calculate the function. 
   *
   * Input: module, cut
   * Output: uncrossable
   */
  void calc_uncrossable(Module *module, const InstSet &cut);
  // Call <calc_uncrossable> before calling this function. 
  // Otherwise all functions are crossable. 
  bool is_uncrossable(Function *f) const { return uncrossable.count(f); }

 private:
  /*
   * Calculate the exit instructions for each function. 
   */
  void calc_exits(Module *module);
  /* 
   * Calculate par_postdomed of <bb>
   * according to the formula mentioned above.
   */
  bool calc_par_postdomed(
      BasicBlock *bb,
      const DenseSet<BasicBlock *> &par_postdomed,
      const InstSet &cut);
  /*
   * Sort basic blocks in a topological order so that we can compute 
   * <uncrossable> easier. 
   * 
   * Note that there may be recursive function calls, and thus
   * the graph of basic blocks may contain cycles. 
   *
   * Therefore, we find SCCs first, and then topologically sort the SCCs
   * and later map them back to basic blocks.
   * 
   * bb_scc maps each basic block to the ID of its belonging SCC. 
   * scc_bbs[i] includes all basic blocks that belong to SCC i. 
   * topo_order is an topological order of these SCCs. 
   *
   * Input: ppg, ppg_r
   * Output: bb_scc, scc_bbs, topo_order 
   */
  void topological_sort(Module &M);
  /*
   * Build the graph only. Need not the cut. 
   *
   * Input: M
   * Output: ppg and ppg_r
   */
  void build_par_postdom_graph(Module &M);
  void make_unique(ParPostdomGraph &pgg);
  void dfs_ppg(
      BasicBlock *x,
      DenseMap<BasicBlock *, int> &finish_time,
      int &now);
  void dfs_ppg_r(BasicBlock *x, DenseSet<BasicBlock *> &visited);
  // <follow_return> indicates if we want to follow return instructions
  // at the same level of <x>. 
  void dfs(
      Instruction *x,
      const InstSet &cut,
      InstSet &visited_nodes,
      InstPairSet &visited_edges,
      bool follow_return);
  // Same as dfs(), but moves backwards. 
  // <follow_call> indicates whether we continue with all call sites
  // when hitting a function entry. 
  void dfs_r(
      Instruction *x,
      const InstSet &cut,
      InstSet &visited_nodes,
      InstPairSet &visited_edges,
      bool follow_call);
  int read_input(
      const string &input_file,
      Instruction *&start,
      InstSet &cut,
      bool &backwards) const;

  // Used for topological_sort
  BBSCCMapping bb_scc;
  SCCBBMapping scc_bbs;
  vector<int> topo_order;
  /* 
   * <ppg> will be the basic block graph used for computing the partial
   * post-dominance relations. <ppg_r> is its reverse (transpose) graph. 
   */
  ParPostdomGraph ppg, ppg_r;
  DenseSet<Function *> uncrossable;
  /*
   * exit_insts[F] contains the exit instructions (ReturnInst or UnwindInst)
   * of function F.
   */
  DenseMap<Function *, vector<Instruction *> > exit_insts;
};
}

#endif
