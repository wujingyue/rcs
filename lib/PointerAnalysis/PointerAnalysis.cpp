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

#include <set>
using namespace std;

#include "llvm/Support/raw_ostream.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/IDAssigner.h"
#include "common/PointerAnalysis.h"
using namespace rcs;

char PointerAnalysis::ID = 0;

static RegisterAnalysisGroup<PointerAnalysis> A("Pointer Analysis");

void PointerAnalysis::printDot(raw_ostream &O, IDAssigner &IDA) {
  O << "strict digraph PointTo {\n";

  set<unsigned> PointerVids, PointeeVids;
  ValueList Pointers;
  getAllPointers(Pointers);
  for (size_t i = 0; i < Pointers.size(); ++i) {
    Value *Pointer = Pointers[i];
    assert(Pointer != NULL);
    assert(Pointer->getType()->isPointerTy());

    unsigned PointerVid = IDA.getValueID(Pointer);
    assert(PointerVid != IDAssigner::INVALID_ID);
    PointerVids.insert(PointerVid);

    ValueList Pointees;
    getPointees(Pointer, Pointees);

    for (size_t j = 0; j < Pointees.size(); ++j) {
      Value *Pointee = Pointees[j];
      assert(Pointee != NULL);

      unsigned PointeeVid = IDA.getValueID(Pointee);
      assert(PointeeVid != IDAssigner::INVALID_ID);
      PointeeVids.insert(PointeeVid);
      
      O << "TopLevel" << PointerVid << " -> AddrTaken" << PointeeVid << "\n";
    }
  }
  
  for (set<unsigned>::iterator I = PointerVids.begin(); I != PointerVids.end();
       ++I) {
    O << "TopLevel" << *I << " ";
    O << "[label = " << *I << "]\n";
  }

  for (set<unsigned>::iterator I = PointeeVids.begin(); I != PointeeVids.end();
       ++I) {
    O << "AddrTaken" << *I << " ";
    O << "[label = " << *I << ", ";
    O << "style = filled, ";
    O << "fillcolor = yellow]\n";
  }

  O << "}\n";
}

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
