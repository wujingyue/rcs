/**
 * Author: Jingyue
 */

#include <iostream>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/CommandLine.h"
#include "common/util.h"
#include "common/IDAssigner.h"
#include "common/src-locator.h"
#include "common/InitializePasses.h"
using namespace llvm;

INITIALIZE_PASS_BEGIN(SourceLocator, "src-locator",
		"From line number to instruction", false, true)
INITIALIZE_PASS_DEPENDENCY(IDAssigner)
INITIALIZE_PASS_END(SourceLocator, "src-locator",
		"From line number to instruction", false, true)

static cl::opt<string> Input("input",
		cl::desc("Input"),
		cl::init(""));

void SourceLocator::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<IDAssigner>();
}

SourceLocator::SourceLocator(): ModulePass(ID) {
	initializeSourceLocatorPass(*PassRegistry::getPassRegistry());
}

bool SourceLocator::runOnModule(Module &M) {
	loc_to_ins.clear();
	ins_to_loc.clear();
	int num_dbgs = 0, num_insts = 0;
	forallinst(M, ii) {
		MDNode *dbg = ii->getMetadata("dbg");
		if (dbg) {
			DILocation loc(dbg);
			unsigned line_no = loc.getLineNumber();
			string file_name = loc.getFilename();
			loc_to_ins[make_pair(file_name, line_no)].push_back(ii);
			ins_to_loc[ii] = make_pair(file_name, line_no);
			num_dbgs++;
		}
		num_insts++;
	}
	errs() << "# of instructions with location info = " << num_dbgs << "\n";
	errs() << "# of instructions = " << num_insts << "\n";

	if (Input != "")
		process_input();

	return false;
}

void SourceLocator::process_input() {
	IDAssigner &IDA = getAnalysis<IDAssigner>();
	string line(Input);
	size_t p = line.find(':');
	if (p != string::npos) {
		string file_name = line.substr(0, p);
		unsigned line_no = atoi(line.substr(p + 1).c_str());
		Instruction *ins = get_first_instruction(file_name, line_no);
		unsigned ins_id = IDA.getInstructionID(ins);
		outs() << ins_id << "\n";
	} else {
		unsigned ins_id = atoi(line.c_str());
		Instruction *ins = IDA.getInstruction(ins_id);
		SourceLoc loc;
		if (!get_location(ins, loc))
			outs() << "Not found\n";
		else
			outs() << loc.first << ":" << loc.second << "\n";
	}
}

Instruction *SourceLocator::get_first_instruction(
		const string &file_name, unsigned line_no) const {
	return get_first_instruction(SourceLoc(file_name, line_no));
}

Instruction *SourceLocator::get_first_instruction(const SourceLoc &loc) const {
	MapLocToIns::const_iterator it = loc_to_ins.find(loc);
	if (it == loc_to_ins.end())
		return NULL;
	if (it->second.empty())
		return NULL;
	return it->second.front();
}

Instruction *SourceLocator::get_last_instruction(const SourceLoc &loc) const {
	MapLocToIns::const_iterator it = loc_to_ins.find(loc);
	if (it == loc_to_ins.end())
		return NULL;
	if (it->second.empty())
		return NULL;
	return it->second.back();
}

Instruction *SourceLocator::get_last_instruction(
		const string &file_name, unsigned line_no) const {
	return get_last_instruction(SourceLoc(file_name, line_no));
}

bool SourceLocator::get_location(
		const Instruction *ins, SourceLoc &loc) const {
	MapInsToLoc::const_iterator it = ins_to_loc.find(ins);
	if (it == ins_to_loc.end())
		return false;
	loc = it->second;
	return true;
}

void SourceLocator::print(raw_ostream &O, const Module *M) const {
	MapLocToIns::const_iterator it = loc_to_ins.begin();
	for (; it != loc_to_ins.end(); ++it) {
		O << it->first.first << ':' << it->first.second
			<< ' ' << it->second.size() << "\n";
	}
}

char SourceLocator::ID = 0;

struct RegisterSourceLocatorPass {
	RegisterSourceLocatorPass() {
		PassRegistry &reg = *PassRegistry::getPassRegistry();
		initializeSourceLocatorPass(reg);
	}
};
static RegisterSourceLocatorPass X;
