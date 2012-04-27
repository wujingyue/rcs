// Author: Jingyue
//
// Checks whether a specified call graph is sound by comparing it with another
// call graph generated on DynamicAliasAnalysis

#include "llvm/Module.h"
#include "llvm/Pass.h"
using namespace llvm;

namespace rcs {
struct CallGraphChecker: public ModulePass {
};
}
