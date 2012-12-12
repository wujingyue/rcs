#include "rcs/FPCallGraph.h"
#include "rcs/IdentifyThreadFuncs.h"
#include "rcs/util.h"
using namespace rcs;

static RegisterPass<IdentifyThreadFuncs> X("identify-thread-funcs",
                                           "Identify thread functions",
                                           false,
                                           true);

char IdentifyThreadFuncs::ID = 0;

void IdentifyThreadFuncs::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<FPCallGraph>();
}

IdentifyThreadFuncs::IdentifyThreadFuncs(): ModulePass(ID) {}

bool IdentifyThreadFuncs::runOnModule(Module &M) {
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator B = F->begin(); B != F->end(); ++B) {
      for (BasicBlock::iterator I = B->begin(); I != B->end(); ++I) {
        // Extract the called thread functions for each pthread_create.
        if (is_pthread_create(I)) {
          if (Value *ThreadFunc = get_pthread_create_callee(I)) {
            if (isa<Function>(ThreadFunc)) {
              // pthread_create with a known function
              ThreadFuncs.insert(cast<Function>(ThreadFunc));
            } else {
              FPCallGraph &CG = getAnalysis<FPCallGraph>();
              FuncList Callees = CG.getCalledFunctions(I);
              for (size_t i = 0; i < Callees.size(); ++i) {
                if (Callees[i]->getName() != "pthread_create")
                  ThreadFuncs.insert(Callees[i]);
              }
            }
          }
        }
      }
    }
  }
  return false;
}

void IdentifyThreadFuncs::print(raw_ostream &O, const Module *M) const {
  O << "Thread functions:\n";
  for (ConstFuncSet::const_iterator I = ThreadFuncs.begin();
       I != ThreadFuncs.end();
       ++I) {
    O << (*I)->getName() << "\n";
  }
}

bool IdentifyThreadFuncs::isThreadFunction(const Function &F) const {
  return ThreadFuncs.count(&F);
}
