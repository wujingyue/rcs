#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

#include "llvm/Module.h"
#include "llvm/LLVMContext.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

#include "rcs/FPCallGraph.h"
#include "rcs/util.h"
#include "rcs/InterReach.h"
#include "rcs/IDAssigner.h"
using namespace rcs;

static RegisterPass<Reachability> X("reach",
                                    "Reachability Analysis", false, true);

char Reachability::ID = 0;

void Reachability::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
  AU.addRequired<FPCallGraph>();
}

Reachability::Reachability(): ModulePass(ID) {}

bool Reachability::runOnModule(Module &M) {
  // Topologically sort BBs in order to calculate par_postdomed more easily. 
  topological_sort(M);

  // Get exit instructions of each function. 
  // An exit instruction is either a ReturnInst or an UnwindInst. 
  calc_exits(&M);

  // Return false since we didn't change the module. 
  return false;
}

bool Reachability::reachable(
    Module *module,
    Instruction *start,
    Instruction *end,
    bool backwards) {
  InstSet visited_nodes;
  InstPairSet visited_edges;
  floodfill(module, start, InstSet(), visited_nodes, visited_edges, backwards);
  return visited_nodes.count(end);
}

bool Reachability::calc_par_postdomed(
    BasicBlock *bb,
    const DenseSet<BasicBlock *> &par_postdomed,
    const InstSet &cut) {
  // Post-dominated if any instruction is in the cut. 
  for (InstSet::const_iterator it = cut.begin();
       it != cut.end(); ++it) {
    if ((*it)->getParent() == bb)
      return true;
  }
  // Iterate through all call instructions in <bb>. 
  FPCallGraph &CG = getAnalysis<FPCallGraph>();
  for (BasicBlock::iterator ii = bb->begin(); ii != bb->end(); ++ii) {
    if (is_call(ii)) {
      const FuncList &callees = CG.getCalledFunctions(ii);
      // All possible targets are blocked, i.e. post-dominated by <cut>. 
      bool all_blocked = true;
      for (size_t k = 0; k < callees.size(); ++k) {
        if (callees[k]->isDeclaration()) {
          all_blocked = false;
          break;
        }
        if (!par_postdomed.count(callees[k]->begin())) {
          all_blocked = false;
          break;
        }
      }
      if (all_blocked)
        return true;
    }
  }
  // If <bb> is a return block and <bb> doesn't call any blocked function,
  // <bb> is not blocked. 
  if (isa<ReturnInst>(bb->getTerminator()) ||
      isa<ResumeInst>(bb->getTerminator()))
    return false;
  // If any successor is not blocked, then <bb> is not blocked. 
  for (succ_iterator it = succ_begin(bb); it != succ_end(bb); ++it) {
    if (!par_postdomed.count(*it))
      return false;
  }
  return true;
}

void Reachability::calc_uncrossable(
    Module *module,
    const InstSet &cut) {
  // <par_postdomed> = true for all BBs initially. 
  DenseSet<BasicBlock *> par_postdomed;
  forallbb((*module), bi)
      par_postdomed.insert(bi);
  // Calculate par_postdomed in the reverse topological order. 
  for (vector<int>::reverse_iterator it = topo_order.rbegin();
       it != topo_order.rend(); ++it) {
    int i = *it; // <i> is the SCC id. 
    bool updated;
    do {
      updated = false;
      const vector<BasicBlock *> &bbs = scc_bbs[i];
      for (size_t j = 0, E = bbs.size(); j < E; ++j) {
        BasicBlock *bb = bbs[j];
        bool old_value = par_postdomed.count(bb);
        // OPT: 
        // par_postdomed[bb] will never go from false to true. 
        // Therefore, if it's already false, we needn't bother calculating it. 
        if (old_value == false)
          continue;
        bool new_value = calc_par_postdomed(bb, par_postdomed, cut);
        // <old_value> must be true. 
        if (!new_value) {
          updated = true;
          par_postdomed.erase(bb);
        }
      }
      // OPT:
      // If the SCC has only one BB, we only need to calculate once. 
    } while (updated && scc_bbs[i].size() > 1);
  }
  // Calculate uncrossable. 
  uncrossable.clear();
  forallfunc(*module, fi) {
    if (fi->isDeclaration())
      continue;
    if (par_postdomed.count(fi->begin()))
      uncrossable.insert(fi);
  }
  DEBUG(dbgs() << "Uncrossable functions:\n";);
  for (DenseSet<Function *>::iterator it = uncrossable.begin();
       it != uncrossable.end(); ++it) {
    DEBUG(dbgs() << (*it)->getName() << "\n";);
  }
}

void Reachability::dfs(
    Instruction *x,
    const InstSet &cut,
    InstSet &visited_nodes,
    InstPairSet &visited_edges,
    bool follow_return) {

  assert(x && "<x> cannot be NULL");
  assert(visited_nodes.count(x));

  FPCallGraph &CG = getAnalysis<FPCallGraph>();
  if (is_call(x)) {
    bool all_blocked = true;
    const FuncList &callees = CG.getCalledFunctions(x);
    for (size_t j = 0, E = callees.size(); j < E; ++j) {
      if (callees[j]->isDeclaration()) {
        all_blocked = false;
        // Don't use break. 
        continue;
      }
      if (!uncrossable.count(callees[j]))
        all_blocked = false;
      Instruction *y = callees[j]->getEntryBlock().begin();
      if (!cut.count(y)) {
        visited_edges.insert(make_pair(x, y));
        if (!visited_nodes.count(y)) {
          visited_nodes.insert(y);
          dfs(y, cut, visited_nodes, visited_edges, false);
        }
      }
    }
    // If cannot go further, stop DFS. 
    if (all_blocked)
      return;
  }

  if (isa<ReturnInst>(x) || isa<ResumeInst>(x)) {
    const vector<Instruction *> &call_sites = CG.getCallSites(
        x->getParent()->getParent());
    for (size_t j = 0, E = call_sites.size(); j < E; ++j) {
      BasicBlock::iterator ret_addr;
      // Get the return address of this ReturnInst or UnwindInst. 
      if (isa<CallInst>(call_sites[j])) {
        // Continue with the next instruction. 
        // A CallInst must not be a terminator. 
        ret_addr = call_sites[j];
        ++ret_addr;
      } else if (isa<InvokeInst>(call_sites[j])) {
        InvokeInst *inv = dyn_cast<InvokeInst>(call_sites[j]);
        // If a regular return, continue with the normal destination;
        // otherwise, continue with the unwind destination. 
        if (isa<ReturnInst>(x))
          ret_addr = inv->getNormalDest()->begin();
        else
          ret_addr = inv->getUnwindDest()->begin();
      } else {
        assert(false && "A call site must be either a CallInst"
               "or an InvokeInst.");
      }
      // Start from the return address. 
      if (!cut.count(ret_addr)) {
        visited_edges.insert(make_pair(x, ret_addr));
        if (follow_return) {
          if (!visited_nodes.count(ret_addr)) {
            visited_nodes.insert(ret_addr);
            dfs(ret_addr, cut, visited_nodes, visited_edges, follow_return);
          }
        }
      }
    }
    // Even if not follow return, we still need visit edges. 
    if (!follow_return)
      return;
  }

  if (!x->isTerminator()) {
    // If <x> is not a terminator, continue with the next instruction. 
    BasicBlock::iterator y = x; ++y;
    if (!cut.count(y)) {
      visited_edges.insert(make_pair(x, y));
      if (!visited_nodes.count(y)) {
        visited_nodes.insert(y);
        dfs(y, cut, visited_nodes, visited_edges, follow_return);
      }
    }
  } else {
    // Continue with all sucessing BBs. 
    BasicBlock *bb = x->getParent();
    for (succ_iterator it = succ_begin(bb); it != succ_end(bb); ++it) {
      Instruction *y = (*it)->begin();
      if (!cut.count(y)) {
        visited_edges.insert(make_pair(x, y));
        if (!visited_nodes.count(y)) {
          visited_nodes.insert(y);
          dfs(y, cut, visited_nodes, visited_edges, follow_return);
        }
      }
    }
  }
}

void Reachability::dfs_r(
    Instruction *x,
    const InstSet &cut,
    InstSet &visited_nodes,
    InstPairSet &visited_edges,
    bool follow_call) {

  assert(x && "<x> cannot be NULL");
  assert(visited_nodes.count(x));
#if 0
  cerr << "dfs_r:";
  x->dump();
#endif

  // Stop if <x> is in the cut. 
  // The current instruction is still visited, because the cut is at
  // the entry. 
  if (cut.count(x))
    return;

  // Function entry. 
  FPCallGraph &CG = getAnalysis<FPCallGraph>();
  if (x == x->getParent()->getParent()->getEntryBlock().begin()) {
    // No problem with going from a function entry to its call site. 
    const vector<Instruction *> &call_sites = CG.getCallSites(
        x->getParent()->getParent());
    // TODO: We could distinguish CallInsts and InvokeInsts here. 
    for (size_t j = 0, E = call_sites.size(); j < E; ++j) {
      Instruction *y = call_sites[j];
      visited_edges.insert(make_pair(y, x));
      if (follow_call) {
        if (!visited_nodes.count(y)) {
          visited_nodes.insert(y);
          dfs_r(y, cut, visited_nodes, visited_edges, follow_call);
        }
      }
    }
    return;
  }

  // Iterate through all potential predecessors. 
  vector<Instruction *> preds;
  if (x != x->getParent()->begin()) {
    // If <x> is not the BB entry, continue with the previous instruction. 
    BasicBlock::iterator prev = x; --prev;
    preds.push_back(prev);
  } else {
    // Continue with all preceding BBs. 
    BasicBlock *bb = x->getParent();
    for (pred_iterator it = pred_begin(bb); it != pred_end(bb); ++it)
      preds.push_back((*it)->getTerminator());
  }

  forall(vector<Instruction *>, it, preds) {
    Instruction *y = *it;
    if (!is_call(y)) {
      // If not a function call, go ahead. 
      visited_edges.insert(make_pair(y, x));
      if (!visited_nodes.count(y)) {
        visited_nodes.insert(y);
        dfs_r(y, cut, visited_nodes, visited_edges, follow_call);
      }
      continue;
    }
    // Is a function call, and reached from the same level (not as above
    // reached from the entry of the callee). 
    // In this case, we need to examine whether <y> is blocked. 
    bool all_blocked = true;
    const FuncList &callees = CG.getCalledFunctions(y);
    for (size_t j = 0, E = callees.size(); j < E; ++j) {
      if (callees[j]->isDeclaration()) {
        all_blocked = false;
        // Don't use break. 
        continue;
      }
      if (!uncrossable.count(callees[j]))
        all_blocked = false;
      // No matter whether <y> is blocked, we need traverse into those callees.
      // TODO: We could distinguish ReturnInst and UnwindInst. 
      const vector<Instruction *> &exits = exit_insts.lookup(callees[j]);
      for (size_t k = 0; k < exits.size(); ++k) {
        Instruction *y = exits[k];
        visited_edges.insert(make_pair(y, x));
        if (!visited_nodes.count(y)) {
          visited_nodes.insert(y);
          dfs_r(y, cut, visited_nodes, visited_edges, false);
        }
      }
    }
    // If <y> is not blocked, we can reach <y> through at least one of its
    // callees. Therefore, we continue DFS from <y>. 
    if (!all_blocked) {
      visited_edges.insert(make_pair(y, x));
      if (!visited_nodes.count(y)) {
        visited_nodes.insert(y);
        dfs_r(y, cut, visited_nodes, visited_edges, follow_call);
      }
    }
  }
}

void Reachability::calc_exits(Module *module) {
  exit_insts.clear();
  for (Module::iterator fi = module->begin(); fi != module->end(); ++fi) {
    for (Function::iterator bi = fi->begin(); bi != fi->end(); ++bi) {
      TerminatorInst *ti = bi->getTerminator();
      if (isa<ReturnInst>(ti) || isa<ResumeInst>(ti))
        exit_insts[fi].push_back(ti);
    }
  }
}

void Reachability::floodfill(
    Module *module,
    Instruction *start,
    const InstSet &cut,
    InstSet &visited_nodes,
    InstPairSet &visited_edges,
    bool backwards) {
  // Calculate uncrossable[F] for each function F. 
  // We can view it as whether we can go across this function without
  // touching any instruction in <cut>. 
  calc_uncrossable(module, cut);
  visited_nodes.clear();
  visited_edges.clear();
  visited_nodes.insert(start);
  if (!backwards)
    dfs(start, cut, visited_nodes, visited_edges, true);
  else
    dfs_r(start, cut, visited_nodes, visited_edges, true);
}

void Reachability::build_par_postdom_graph(Module &M) {
  FPCallGraph &CG = getAnalysis<FPCallGraph>();
  ppg.clear(); ppg_r.clear();
  forallbb(M, bi) {
    // Edge: bi => the entry block of every function it calls. 
    for (BasicBlock::iterator ii = bi->begin(); ii != bi->end(); ++ii) {
      if (is_call(ii)) {
        const FuncList &callees = CG.getCalledFunctions(ii);
        for (size_t j = 0, E = callees.size(); j < E; ++j) {
          // Skip empty functions because they don't have any BBs. 
          if (callees[j]->isDeclaration())
            continue;
          ppg[bi].push_back(callees[j]->begin());
          ppg_r[callees[j]->begin()].push_back(bi);
        }
      }
    }
    // Edge: bi => each of its successors
    for (succ_iterator it = succ_begin(bi); it != succ_end(bi); ++it) {
      ppg[bi].push_back(*it);
      ppg_r[*it].push_back(bi);
    }
  }
  // Remove duplicated items in each vector in <ppg> and <ppg_r>. 
  make_unique(ppg);
  make_unique(ppg_r);
}

void Reachability::make_unique(ParPostdomGraph &ppg) {
  for (ParPostdomGraph::iterator it = ppg.begin(); it != ppg.end(); ++it) {
    sort(it->second.begin(), it->second.end());
    vector<BasicBlock *>::iterator new_end = unique(
        it->second.begin(),
        it->second.end());
    it->second.resize(new_end - it->second.begin());
  }
}

void Reachability::dfs_ppg(
    BasicBlock *x,
    DenseMap<BasicBlock *, int> &finish_time,
    int &now) {
  if (finish_time.count(x))
    return;
  finish_time[x] = -1; // Discovered but not finished yet. 
  const vector<BasicBlock *> &nbrs = ppg.lookup(x);
  for (size_t j = 0, E = nbrs.size(); j < E; ++j)
    dfs_ppg(nbrs[j], finish_time, now);
  finish_time[x] = now;
  now++;
}

void Reachability::dfs_ppg_r(BasicBlock *x, DenseSet<BasicBlock *> &visited) {
  assert(!visited.count(x) && "<x> is already visited");
  visited.insert(x);
  bb_scc[x] = (int)scc_bbs.size() - 1;
  scc_bbs.back().push_back(x);
  const vector<BasicBlock *> &nbrs = ppg_r.lookup(x);
  for (size_t j = 0, E = nbrs.size(); j < E; ++j) {
    BasicBlock *bb = nbrs[j];
    if (!visited.count(bb))
      dfs_ppg_r(nbrs[j], visited);
  }
}

void Reachability::topological_sort(Module &M) {
  // Build the partial post-dominance graph. 
  build_par_postdom_graph(M);
  // Find all SCCs. 
  DenseMap<BasicBlock *, int> finish_time;
  int now; // timestamp
  // Get the finish times. 
  forallbb(M, bi)
      dfs_ppg(bi, finish_time, now);
  // Sort BBs according to the finish time. 
  vector<pair<int, BasicBlock *> > order;
  for (DenseMap<BasicBlock *, int>::iterator it = finish_time.begin();
       it != finish_time.end(); ++it) {
    order.push_back(make_pair(it->second, it->first));
  }
  sort(order.begin(), order.end(), greater<pair<int, BasicBlock *> >());
  // DFS again in order of decreasing finish_time[u]. 
  // Output the vertices of each tree as a SCC. 
  DenseSet<BasicBlock *> visited;
  scc_bbs.clear();
  for (size_t i = 0, E = order.size(); i < E; ++i) {
    BasicBlock *bb = order[i].second;
    if (!visited.count(bb)) {
      scc_bbs.push_back(vector<BasicBlock *>());
      dfs_ppg_r(bb, visited);
    }
  }
  // Calculate finish_time(C) for each SCC C. 
  // finish_time(C) = max_{u in C}(finish_time(u))
  vector<pair<int, int> > maxf_scc;
  for (size_t i = 0, E = scc_bbs.size(); i < E; ++i) {
    int maxf = 0;
    for (size_t j = 0; j < scc_bbs[i].size(); ++j)
      maxf = max(maxf, finish_time[scc_bbs[i][j]]);
    maxf_scc.push_back(make_pair(maxf, i));
  }
  assert(maxf_scc.size() == scc_bbs.size());
  // Sort maxf_scc by the descreasing order of maxf
  // This order is exactly the topological order we want ultimately
  sort(maxf_scc.begin(), maxf_scc.end(), greater<pair<int, int> >());
  // Output to <topo_order>
  topo_order.clear();
  for (size_t i = 0, E = maxf_scc.size(); i < E; ++i)
    topo_order.push_back(maxf_scc[i].second);
}

int Reachability::read_input(
    const string &input_file,
    Instruction *&start,
    InstSet &cut,
    bool &backwards) const {
  string line;
  istringstream iss;
  int i;

  IDAssigner &IDA = getAnalysis<IDAssigner>();

  ifstream fin(input_file.c_str());
  if (!fin) {
    cerr << "Cannot find file " << input_file << endl;
    return -1;
  }

  getline(fin, line);
  iss.str(line);
  iss >> i;
  start = IDA.getInstruction(i);
  if (start == NULL) {
    cerr << "Instruction " << i << " doesn't exist.\n";
    return -1;
  }

  getline(fin, line);
  iss.clear();
  iss.str(line);
  while (iss >> i) {
    Instruction *ins = IDA.getInstruction(i);
    if (ins == NULL) {
      cerr << "Instruction " << i << " doesn't exist.\n";
      return -1;
    }
    cut.insert(ins);
  }

  getline(fin, line);
  iss.clear();
  iss.str(line);
  iss >> i;
  backwards = (i != 0);

  return 0;
}
