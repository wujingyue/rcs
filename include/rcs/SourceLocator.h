#ifndef __RCS_SOURCE_LOCATOR_H
#define __RCS_SOURCE_LOCATOR_H

#include <vector>
#include <set>
#include <map>
#include <fstream>

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Instructions.h"

namespace rcs {
struct SourceLocator: public llvm::ModulePass {
  typedef std::pair<std::string, unsigned> SourceLoc;
  typedef llvm::DenseMap<SourceLoc, std::vector<llvm::Instruction *> >
      LocToInsMapTy;
  typedef llvm::DenseMap<const llvm::Instruction *, SourceLoc> InsToLocMapTy;

  static char ID;

  SourceLocator();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &O, const Module *M) const;

  llvm::Instruction *getFirstInstruction(const std::string &file_name,
                                         unsigned line_no) const;
  llvm::Instruction *getFirstInstruction(const SourceLoc &loc) const;
  llvm::Instruction *getLastInstruction(const std::string &file_name,
                                        unsigned line_no) const;
  llvm::Instruction *getLastInstruction(const SourceLoc &loc) const;
  bool getLocation(const llvm::Instruction *ins, SourceLoc &loc) const;

 private:
  void processInput();

  LocToInsMapTy LocToIns;
  InsToLocMapTy InsToLoc;
};
}

#endif
