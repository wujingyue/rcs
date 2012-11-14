#include <iostream>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

#include "rcs/util.h"
#include "rcs/IDAssigner.h"
#include "rcs/SourceLocator.h"
using namespace rcs;

static RegisterPass<SourceLocator> X("locate-src",
                                     "From line number to instruction", false, true);

static cl::opt<string> Input("pos", cl::desc("Input"));

void SourceLocator::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<IDAssigner>();
}

SourceLocator::SourceLocator(): ModulePass(ID) {}

bool SourceLocator::runOnModule(Module &M) {
  LocToIns.clear();
  InsToLoc.clear();
  int NumDbgs = 0, NumInsts = 0;
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
      for (BasicBlock::iterator Ins = BB->begin(); Ins != BB->end(); ++Ins) {
        MDNode *Dbg = Ins->getMetadata("dbg");
        if (Dbg) {
          DILocation Loc(Dbg);
          unsigned LineNo = Loc.getLineNumber();
          string FileName = Loc.getFilename();
          LocToIns[make_pair(FileName, LineNo)].push_back(Ins);
          InsToLoc[Ins] = make_pair(FileName, LineNo);
          NumDbgs++;
        }
        NumInsts++;
      }
    }
  }
  errs() << "# of instructions with location info = " << NumDbgs << "\n";
  errs() << "# of instructions = " << NumInsts << "\n";

  if (Input != "")
    processInput();

  return false;
}

void SourceLocator::processInput() {
  IDAssigner &IDA = getAnalysis<IDAssigner>();
  string Line(Input);
  size_t Pos = Line.find(':');
  if (Pos != string::npos) {
    string FileName = Line.substr(0, Pos);
    unsigned LineNo = atoi(Line.substr(Pos + 1).c_str());
    Instruction *Ins = getFirstInstruction(FileName, LineNo);
    unsigned InsID = IDA.getInstructionID(Ins);
    outs() << InsID << "\n";
  } else {
    Instruction *Ins = NULL;
    if (Line[0] == 'i') {
      // i<ins ID>
      unsigned InsID = atoi(Line.c_str() + 1);
      Ins = IDA.getInstruction(InsID);
    } else {
      assert(Line[0] == 'v');
      unsigned ValueID = atoi(Line.c_str() + 1);
      Ins = dyn_cast<Instruction>(IDA.getValue(ValueID));
    }
    SourceLoc Loc;
    if (!Ins || !getLocation(Ins, Loc))
      outs() << "Not found\n";
    else
      outs() << Loc.first << ":" << Loc.second << "\n";
  }
}

Instruction *SourceLocator::getFirstInstruction(const string &FileName,
                                                unsigned LineNo) const {
  return getFirstInstruction(SourceLoc(FileName, LineNo));
}

Instruction *SourceLocator::getFirstInstruction(const SourceLoc &Loc) const {
  LocToInsMapTy::const_iterator I = LocToIns.find(Loc);
  if (I == LocToIns.end())
    return NULL;
  if (I->second.empty())
    return NULL;
  return I->second.front();
}

Instruction *SourceLocator::getLastInstruction(const SourceLoc &Loc) const {
  LocToInsMapTy::const_iterator I = LocToIns.find(Loc);
  if (I == LocToIns.end())
    return NULL;
  if (I->second.empty())
    return NULL;
  return I->second.back();
}

Instruction *SourceLocator::getLastInstruction(const string &FileName,
                                               unsigned LineNo) const {
  return getLastInstruction(SourceLoc(FileName, LineNo));
}

bool SourceLocator::getLocation(
    const Instruction *ins, SourceLoc &Loc) const {
  InsToLocMapTy::const_iterator I = InsToLoc.find(ins);
  if (I == InsToLoc.end())
    return false;
  Loc = I->second;
  return true;
}

void SourceLocator::print(raw_ostream &O, const Module *M) const {
  for (LocToInsMapTy::const_iterator I = LocToIns.begin();
       I != LocToIns.end(); ++I) {
    O << I->first.first << ':' << I->first.second
        << ' ' << I->second.size() << "\n";
  }
}

char SourceLocator::ID = 0;
