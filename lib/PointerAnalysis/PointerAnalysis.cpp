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

#include <set>
using namespace std;

#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include "rcs/IDAssigner.h"
#include "rcs/PointerAnalysis.h"
using namespace rcs;

char PointerAnalysis::ID = 0;

static RegisterAnalysisGroup<PointerAnalysis> A("Pointer Analysis");

void PointerAnalysis::printStats(raw_ostream &O) {
  ValueList Pointers;
  getAllPointers(Pointers);
  O << "# of pointers = " << Pointers.size() << "\n";

  unsigned NumPointTos = 0;
  for (size_t i = 0; i < Pointers.size(); ++i) {
    ValueList Pointees;
    getPointees(Pointers[i], Pointees);
    NumPointTos += Pointees.size();
  }
  O << "# of point-to relations = " << NumPointTos << "\n";
}
