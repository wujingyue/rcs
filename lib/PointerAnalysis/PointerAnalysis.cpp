// Author: Jingyue

// PointerAnalysis is an abstract interface for any pointer analysis. 
// It has the interface <getPointees> which takes a pointer and returns
// the set of all pointees of this pointer. 
// 
// PointerAnalysis is not a pass itself, which is similar to AliasAnalysis.
// Any inherited class of PointerAnalysis should implement their own point-to
// analysis if necessary. 
//
// PointerAnalysis is a AnalysisGroup. The default instance of this group
// is BasicPointerAnalysis. 

#include "llvm/Support/raw_ostream.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/PointerAnalysis.h"
using namespace rcs;

char PointerAnalysis::ID = 0;

#if 0
INITIALIZE_ANALYSIS_GROUP(PointerAnalysis,
                          "Pointer Analysis",
                          BasicPointerAnalysis)
#endif

#if 1
static RegisterAnalysisGroup<PointerAnalysis> A("Pointer Analysis");
#endif

#if 0
struct RegisterPointerAnalysisPasses {
  RegisterPointerAnalysisPasses() {
    PassRegistry &Reg = *PassRegistry::getPassRegistry();
    initializePointerAnalysisAnalysisGroup(Reg);
    initializeBasicPointerAnalysisPass(Reg);
  }
};
static RegisterPointerAnalysisPasses X;
#endif
