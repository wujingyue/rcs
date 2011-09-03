#include <vector>
using namespace std;

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/AliasAnalysis.h"
using namespace llvm;


namespace llvm_common {
	struct AliasPairs: public ModulePass {
		static char ID;

		AliasPairs(): ModulePass(ID) {}
		
		virtual bool runOnModule(Module &M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;
		virtual void print(raw_ostream &O, const Module *M) const;

	private:
		void print_inst(raw_ostream &O, Instruction *ins) const;
		vector<pair<Instruction *, Instruction *> > ww_races, rw_races;
	};
}
using namespace llvm_common;

static RegisterPass<llvm_common::AliasPairs> X("alias-pairs",
		"An LLVM pass dumping all alias pairs",
		false, true);

char AliasPairs::ID = 0;

void AliasPairs::print_inst(raw_ostream &O, Instruction *ins) const {
	BasicBlock *bb = ins->getParent();
	Function *f = bb->getParent();
	O << f->getNameStr() << ":" << bb->getNameStr() << ":" << *ins << "\n";
}

void AliasPairs::print(raw_ostream &O, const Module *M) const {
	O << "\nWrite-write potential races:\n";
	for (vector<pair<Instruction *, Instruction *> >::const_iterator
			itr = ww_races.begin(); itr != ww_races.end(); ++itr) {
		O << string(10, '=') << "\n";
		print_inst(O, itr->first);
		print_inst(O, itr->second);
	}
	O << "\nRead-write potential races:\n";
	for (vector<pair<Instruction *, Instruction *> >::const_iterator
			itr = rw_races.begin(); itr != rw_races.end(); ++itr) {
		O << string(10, '=') << "\n";
		print_inst(O, itr->first);
		print_inst(O, itr->second);
	}
}

bool AliasPairs::runOnModule(Module &M) {
	vector<LoadInst *> rd_insts;
	vector<StoreInst *> wr_insts;
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
			for (BasicBlock::iterator ins = bb->begin(); ins != bb->end(); ++ins) {
				if (StoreInst *si = dyn_cast<StoreInst>(ins))
					wr_insts.push_back(si);
				if (LoadInst *li = dyn_cast<LoadInst>(ins))
					rd_insts.push_back(li);
			}
		}
	}

	AliasAnalysis &AA = getAnalysis<AliasAnalysis>();

	for (size_t i = 0; i < wr_insts.size(); ++i) {
		for (size_t j = i + 1; j < wr_insts.size(); ++j) {
			if (AA.alias(wr_insts[i]->getPointerOperand(), 0,
						wr_insts[j]->getPointerOperand(), 0)) {
				ww_races.push_back(make_pair(wr_insts[i], wr_insts[j]));
			}
		}
	}

	for (size_t i = 0; i < wr_insts.size(); ++i) {
		for (size_t j = 0; j < rd_insts.size(); ++j) {
			if (AA.alias(wr_insts[i]->getPointerOperand(), 0,
						rd_insts[j]->getPointerOperand(), 0)) {
				rw_races.push_back(make_pair(wr_insts[i], rd_insts[j]));
			}
		}
	}

	return false;
}

void AliasPairs::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<AliasAnalysis>();
	ModulePass::getAnalysisUsage(AU);
}
