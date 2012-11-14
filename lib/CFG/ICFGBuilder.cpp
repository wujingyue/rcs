#include "llvm/Support/CFG.h"
using namespace llvm;

#include "rcs/ICFGBuilder.h"
#include "rcs/FPCallGraph.h"
#include "rcs/util.h"
using namespace rcs;

static RegisterPass<ICFGBuilder> X("icfg",
                                   "Build inter-procedural control flow graph",
                                   false,
                                   true);

ICFGBuilder::ICFGBuilder(): ModulePass(ID) {}

void ICFGBuilder::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<MicroBasicBlockBuilder>();
  AU.addRequired<FPCallGraph>();
}

bool ICFGBuilder::runOnModule(Module &M) {
  MicroBasicBlockBuilder &MBBB = getAnalysis<MicroBasicBlockBuilder>();

  forallbb(M, bb) {
    for (mbb_iterator mi = MBBB.begin(bb), E = MBBB.end(bb); mi != E; ++mi)
      getOrInsertMBB(mi);
  }

  forallbb(M, bb) {
    for (mbb_iterator mi = MBBB.begin(bb), E = MBBB.end(bb); mi != E; ++mi) {
      // The ICFG will not contain any inter-thread edge. 
      // It's also difficult to handle them. How to deal with the return
      // edges? They are supposed to go to the pthread_join sites. 
      if (mi->end() != bb->end() && !is_pthread_create(mi->end())) {
        FPCallGraph &CG = getAnalysis<FPCallGraph>();
        FuncList callees = CG.getCalledFunctions(mi->end());
        bool calls_decl = false;
        for (size_t i = 0; i < callees.size(); ++i) {
          Function *callee = callees[i];
          if (callee->isDeclaration()) {
            calls_decl = true;
          } else {
            MicroBasicBlock *entry_mbb = MBBB.begin(callee->begin());
            addEdge(mi, entry_mbb);
          }
        }
        if (calls_decl) {
          mbb_iterator next_mbb = mi; ++next_mbb;
          addEdge(mi, next_mbb);
        }
      } else {
        for (succ_iterator si = succ_begin(bb); si != succ_end(bb); ++si) {
          MicroBasicBlock *succ_mbb = MBBB.begin(*si);
          addEdge(mi, succ_mbb);
        }
        TerminatorInst *ti = bb->getTerminator();
        if (is_ret(ti)) {
          FPCallGraph &CG = getAnalysis<FPCallGraph>();
          InstList call_sites = CG.getCallSites(bb->getParent());
          for (size_t i = 0; i < call_sites.size(); ++i) {
            Instruction *call_site = call_sites[i];
            // Ignore inter-thread edges. 
            if (is_pthread_create(call_site))
              continue;
            MicroBasicBlock *next_mbb;
            if (isa<CallInst>(call_site)) {
              BasicBlock::iterator next = call_site;
              ++next;
              next_mbb = MBBB.parent(next);
            } else {
              assert(isa<InvokeInst>(call_site));
              InvokeInst *inv = dyn_cast<InvokeInst>(call_site);
              if (isa<ReturnInst>(ti)) {
                next_mbb = MBBB.begin(inv->getNormalDest());
              } else {
                next_mbb = MBBB.begin(inv->getUnwindDest());
              }
            }
            addEdge(mi, next_mbb);
          }
        }
      }
    }
  }
  return false;
}

char ICFGBuilder::ID = 0;
