/* -*- Mode: C++ -*- */

#ifndef __UTIL_H
#define __UTIL_H

#include <cassert>
#include <string>
using namespace std;

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Instructions.h"
#include "llvm/Function.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"
using namespace llvm;

#define forit(it, c) for ((it) = (c).begin(); (it) != (c).end(); ++(it))

#define forall(type, it, c) for (type::iterator it = (c).begin(), \
                                 E = (c).end(); (it) != E; ++(it))

#define forallconst(type, it, c) for (type::const_iterator it = (c).begin(), \
                                      E = (c).end(); (it) != E; ++(it))

#define fori(i,c) for ((i) = 0; (i) < (int)(c).size(); ++(i))

#define forallfunc(M, fi) \
    forall(Module, fi, M)

#define forallbb(M, bi) \
    forall(Module, fi, M) \
      forall(Function, bi, *fi)

#define forallinst(M, ii) \
    forall(Module, fi, M) \
      forall(Function, bi, *fi) \
        forall(BasicBlock, ii, *bi)

#define assert_not_implemented() assert(false && "Not implemented")
#define assert_not_supported() assert(false && "Not supported")
#define assert_unreachable() assert(false && "Unreachable")

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

namespace llvm {
template <> struct DenseMapInfo<string> {
static inline string getEmptyKey() { return ""; }
static inline string getTombstoneKey() { return "Tombstone"; }
static unsigned getHashValue(const string &S) {
  unsigned Result = 0;
  for (size_t I = 0; I < S.length(); ++I)
    Result = Result * 37 + (unsigned)(S[I]);
  return Result;
}
static bool isEqual(const string &S1, const string &S2) {
  return S1 == S2;
}
};
}

namespace rcs {
template<class I, class C>
static inline bool exist(const I& i, const C& c) {
  return c.find(i) != c.end();
}

static inline bool is_call(const Instruction *I) {
  return isa<CallInst>(I) || isa<InvokeInst>(I);
}

static inline bool is_ret(const Instruction *I) {
  return isa<ReturnInst>(I) || isa<ResumeInst>(I);
}

// We could make it more robust by using CallSite::get and isIntrinsicInst.
static inline bool is_intrinsic_call(const Instruction *I) {
  const CallInst *ci = dyn_cast<CallInst>(I);
  if (!ci)
    return false;
  const Function *callee = ci->getCalledFunction();
  return (callee && callee->isIntrinsic());
}

static inline bool is_non_intrinsic_call(const Instruction *I) {
  return is_call(I) && !is_intrinsic_call(I);
}

static inline bool is_pthread_create(const Instruction *I) {
  CallSite cs(const_cast<Instruction *>(I));
  // Not even a call/invoke. 
  if (!cs.getInstruction())
    return false;
  const Function *callee = cs.getCalledFunction();
  if (!callee)
    return false;
  if (callee->getName() == "pthread_create")
    return cs.arg_size() == 4;
  else if (callee->getName() == "tern_wrap_pthread_create")
    return cs.arg_size() == 6;
  else
    return false;
}

static inline Value *get_pthread_create_callee(const Instruction *I) {
  assert(is_pthread_create(I));
  CallSite cs(const_cast<Instruction *>(I));
  return cs.getArgument(cs.arg_size() == 4 ? 2 : 4);
}

static inline Value *get_pthread_create_arg(const Instruction *I) {
  assert(is_pthread_create(I));
  CallSite cs(const_cast<Instruction *>(I));
  return cs.getArgument(cs.arg_size() == 4 ? 3 : 5);
}

static inline void set_pthread_create_callee(Instruction *I, Value *callee) {
  assert(is_pthread_create(I));
  CallSite cs(I);
  cs.setArgument((cs.arg_size() == 4 ? 2 : 4), callee);
}

static inline void set_pthread_create_arg(Instruction *I, Value *arg) {
  assert(is_pthread_create(I));
  CallSite cs(I);
  cs.setArgument((cs.arg_size() == 4 ? 3 : 5), arg);
}

static inline bool is_main(const Function *f) {
  return f->getName() == "main" && !f->hasLocalLinkage();
}

static inline bool starts_with(const string &a, const string &b) {
  return a.find(b) == 0;
}

static inline bool is_function_entry(const Instruction *i) {
  if (!i)
    return false;
  const Function *f = i->getParent()->getParent();
  return i == f->begin()->begin();
}
}

#endif
