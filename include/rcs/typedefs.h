#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include <vector>

#include "llvm/Function.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DenseMap.h"

#include "rcs/MBB.h"

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

typedef std::vector<Function *> FuncList;
typedef std::vector<BasicBlock *> BBList;
typedef std::vector<MicroBasicBlock *> MBBList;
typedef std::vector<Instruction *> InstList;
typedef std::vector<Value *> ValueList;

typedef std::pair<Instruction *, Instruction *> InstPair;
typedef std::pair<Value *, Value *> ValuePair;
typedef std::pair<Use *, Use *> UsePair;
typedef std::pair<BasicBlock *, BasicBlock *> BBPair;

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

typedef std::vector<const Function *> ConstFuncList;
typedef std::vector<const BasicBlock *> ConstBBList;
typedef std::vector<const MicroBasicBlock *> ConstMBBList;
typedef std::vector<const Instruction *> ConstInstList;
typedef std::vector<const Value *> ConstValueList;

typedef std::pair<const Instruction *, const Instruction *> ConstInstPair;
typedef std::pair<const Value *, const Value *> ConstValuePair;
typedef std::pair<const Use *, const Use *> ConstUsePair;
}

#endif
