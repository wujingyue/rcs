/**
 * Author: Jingyue
 */

#ifndef __SOURCE_LOCATOR_H
#define __SOURCE_LOCATOR_H

#include <vector>
#include <set>
#include <map>
#include <fstream>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Instructions.h"
using namespace llvm;

namespace rcs {
	struct SourceLocator: public ModulePass {
		static char ID;

		typedef pair<string, unsigned> SourceLoc;
		typedef DenseMap<SourceLoc, vector<Instruction *> > MapLocToIns;
		typedef DenseMap<const Instruction *, SourceLoc> MapInsToLoc;

		SourceLocator();
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual bool runOnModule(Module &M);
		virtual void print(raw_ostream &O, const Module *M) const;

		Instruction *get_first_instruction(
				const string &file_name, unsigned line_no) const;
		Instruction *get_first_instruction(const SourceLoc &loc) const;
		Instruction *get_last_instruction(
				const string &file_name, unsigned line_no) const;
		Instruction *get_last_instruction(const SourceLoc &loc) const;
		bool get_location(const Instruction *ins, SourceLoc &loc) const;

	private:
		void process_input();

		MapLocToIns loc_to_ins;
		MapInsToLoc ins_to_loc;
	};
}

#endif
