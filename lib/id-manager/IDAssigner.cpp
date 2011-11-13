/**
 * Author: Jingyue
 */

#define DEBUG_TYPE "assign-id"

#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "common/util.h"
#include "common/IDAssigner.h"
using namespace llvm;

#include <fstream>
using namespace std;

static RegisterPass<IDAssigner> X("assign-id",
		"Assign a unique ID to each instruction and each value",
		false, true);

static cl::opt<bool> PrintInsts("print-insts",
		cl::desc("Print the ID-instruction mapping"));
static cl::opt<bool> PrintValues("print-values",
		cl::desc("Print the ID-value mapping"));

STATISTIC(NumInstructions, "Number of instructions");
STATISTIC(NumValues, "Number of values");

char IDAssigner::ID = 0;
const unsigned IDAssigner::INVALID_ID;

bool IDAssigner::addValue(Value *V) {
	if (ValueIDMapping.count(V))
		return true;

	unsigned ValueID = ValueIDMapping.size();
	ValueIDMapping[V] = ValueID;
	IDValueMapping[ValueID] = V;
	++NumValues;
	return false;
}

bool IDAssigner::addIns(Instruction *I) {
	if (InsIDMapping.count(I))
		return true;

	unsigned InsID = InsIDMapping.size();
	InsIDMapping[I] = InsID;
	IDInsMapping[InsID] = I;
	++NumInstructions;
	return false;
}

bool IDAssigner::addFunction(Function *F) {
	if (FunctionIDMapping.count(F))
		return true;

	unsigned FunctionID = FunctionIDMapping.size();
	FunctionIDMapping[F] = FunctionID;
	IDFunctionMapping[FunctionID] = F;
	return false;
}

bool IDAssigner::runOnModule(Module &M) {
	NumInstructions = 0;
	NumValues = 0;
	InsIDMapping.clear();
	ValueIDMapping.clear();
	FunctionIDMapping.clear();
	IDInsMapping.clear();
	IDValueMapping.clear();
	IDFunctionMapping.clear();

	for (Module::iterator F = M.begin(); F != M.end(); ++F) {
		addFunction(F);
		addValue(F);
		for (Function::iterator BB = F->begin(); BB != F->end(); ++BB) {
			addValue(BB);
			for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I) {
				addIns(I);
				// Recursively extract operands as well. 
				extractValuesInUser(I);
			}
		}
	}

	// Global variables
	for (Module::global_iterator G = M.global_begin();
			G != M.global_end(); ++G) {
		addValue(G);
		if (G->hasInitializer())
			extractValuesInUser(G->getInitializer());
	}

	// Function parameters
	for (Module::iterator F = M.begin(); F != M.end(); ++F) {
		for (Function::arg_iterator AI = F->arg_begin();
				AI != F->arg_end(); ++AI) {
			addValue(AI);
		}
	}

	// We don't handle intrinsic values (e.g. metadata).
	return false;
}

void IDAssigner::extractValuesInUser(User *U) {
	// If <v> already exists, don't go recursively. 
	if (addValue(U))
		return;
	for (User::op_iterator OI = U->op_begin(); OI != U->op_end(); ++OI) {
		Value *V = OI->get();
		if (User *U2 = dyn_cast<User>(V)) {
			extractValuesInUser(U2);
		} else {
			// FIXME: LLVM's bitcode writer sometimes modifies the MDNodes.
			// Don't assign them IDs, otherwise might be inconsistent. 
			if (!isa<MDNode>(V))
				addValue(V);
		}
	}
}

unsigned IDAssigner::getValueID(const Value *V) const {
	if (ValueIDMapping.count(V))
		return ValueIDMapping.lookup(V);
	return INVALID_ID;
}

unsigned IDAssigner::getFunctionID(const Function *F) const {
	DenseMap<const Function *, unsigned>::const_iterator itr =
		FunctionIDMapping.find(F);
	return (itr == FunctionIDMapping.end() ? INVALID_ID : itr->second);
}

unsigned IDAssigner::getInstructionID(const Instruction *I) const {
	if (InsIDMapping.count(I))
		return InsIDMapping.lookup(I);
	return INVALID_ID;
}

Value *IDAssigner::getValue(unsigned ID) const {
	return IDValueMapping.lookup(ID);
}

Instruction *IDAssigner::getInstruction(unsigned ID) const {
	return IDInsMapping.lookup(ID);
}

Function *IDAssigner::getFunction(unsigned ID) const {
	return IDFunctionMapping.lookup(ID);
}

void IDAssigner::printInstructions(raw_ostream &O, const Module *M) const {
	O << "Printing the ID-instruction mapping...\n";
	vector<pair<unsigned, const Instruction *> > All;
	DenseMap<const Instruction *, unsigned>::const_iterator I;
	for (I = InsIDMapping.begin(); I != InsIDMapping.end(); ++I)
		All.push_back(make_pair(I->second, I->first));
	sort(All.begin(), All.end());
	// Instruction IDs are consecutive and start from 0. 
	for (size_t i = 0, E = All.size(); i < E; ++i) {
		if (i % 1000 == 0)
			errs() << "Progress: " << i << "/" << All.size() << "\n";
		assert(All[i].first == i && "Instruction IDs are not consecutive or "
				"don't start from 0.");
		const Instruction *Ins = All[i].second;
		const BasicBlock *BB = Ins->getParent();
		const Function *F = BB->getParent();
		// Print the function name if <ins> is the function entry. 
		if (Ins == F->getEntryBlock().begin()) 
			O << "\nFunction " << F->getName() << ":\n";
		if (Ins == BB->begin())
			O << "\nBB " << F->getName() << "." << BB->getName() << ":\n";
		O << i << ":" << *Ins << "\n";
	}	
	errs() << "Progress: " << All.size() << "/" << All.size() << "\n";
}

void IDAssigner::printValues(raw_ostream &O, const Module *M) const {
	O << "Printing the ID-value mapping...\n";
	vector<pair<unsigned, const Value *> > All;
	DenseMap<const Value *, unsigned>::const_iterator I;
	for (I = ValueIDMapping.begin(); I != ValueIDMapping.end(); ++I)
		All.push_back(make_pair(I->second, I->first));
	sort(All.begin(), All.end());
	// Value IDs are consecutive and start from 0. 
	for (size_t i = 0, E = All.size(); i < E; ++i) {
		if (i % 1000 == 0)
			errs() << "Progress: " << i << "/" << All.size() << "\n";
		assert(All[i].first == i && "Value IDs are not consecutive or "
				"don't start from 0.");
		const Value *V = All[i].second;
		O << i << ":\t";
		printValue(O, V);
		O << "\n";
	}	
	errs() << "Progress: " << All.size() << "/" << All.size() << "\n";
}

void IDAssigner::printValue(raw_ostream &O, const Value *V) const {
	if (const Function *F = dyn_cast<Function>(V))
		O << F->getName();
	else if (const BasicBlock *BB = dyn_cast<BasicBlock>(V))
		O << BB->getParent()->getName() << "." << BB->getName();
	else if (const Instruction *I = dyn_cast<Instruction>(V))
		O << I->getParent()->getParent()->getName() << " " << *I;
	else
		V->print(O);
}

void IDAssigner::print(raw_ostream &O, const Module *M) const {
	if (PrintInsts)
		printInstructions(O, M);
	if (PrintValues)
		printValues(O, M);
}
