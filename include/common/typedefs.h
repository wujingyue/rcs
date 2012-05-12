#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include <vector>
using namespace std;

#include "llvm/Function.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DenseMap.h"
#include "common/MBB.h"
using namespace llvm;

namespace rcs {
	typedef DenseMap<Function *, Function *> FuncMapping;
	typedef DenseMap<BasicBlock *, BasicBlock *> BBMapping;
	typedef DenseMap<MicroBasicBlock *, MicroBasicBlock *> MBBMapping;
	typedef DenseMap<Instruction *, Instruction *> InstMapping;
	typedef DenseMap<Value *, Value *> ValueMapping;

	typedef DenseSet<Function *> FuncSet;
	typedef DenseSet<BasicBlock *> BBSet;
	typedef DenseSet<MicroBasicBlock *> MBBSet;
	typedef DenseSet<Instruction *> InstSet;
	typedef DenseSet<Value *> ValueSet;

	typedef vector<Function *> FuncList;
	typedef vector<BasicBlock *> BBList;
	typedef vector<MicroBasicBlock *> MBBList;
	typedef vector<Instruction *> InstList;
	typedef vector<Value *> ValueList;

	typedef pair<Instruction *, Instruction *> InstPair;
	typedef pair<Value *, Value *> ValuePair;
	typedef pair<Use *, Use *> UsePair;

	typedef DenseMap<const Function *, const Function *> ConstFuncMapping;
	typedef DenseMap<const BasicBlock *, const BasicBlock *> ConstBBMapping;
	typedef DenseMap<const MicroBasicBlock *,
					const MicroBasicBlock *> ConstMBBMapping;
	typedef DenseMap<const Instruction *, const Instruction *> ConstInstMapping;
	typedef DenseMap<const Value *, const Value *> ConstValueMapping;

	typedef DenseSet<const Function *> ConstFuncSet;
	typedef DenseSet<const BasicBlock *> ConstBBSet;
	typedef DenseSet<const MicroBasicBlock *> ConstMBBSet;
	typedef DenseSet<const Instruction *> ConstInstSet;
	typedef DenseSet<const Value *> ConstValueSet;

	typedef vector<const Function *> ConstFuncList;
	typedef vector<const BasicBlock *> ConstBBList;
	typedef vector<const MicroBasicBlock *> ConstMBBList;
	typedef vector<const Instruction *> ConstInstList;
	typedef vector<const Value *> ConstValueList;

	typedef pair<const Instruction *, const Instruction *> ConstInstPair;
	typedef pair<const Value *, const Value *> ConstValuePair;
	typedef pair<const Use *, const Use *> ConstUsePair;
}

#endif
