/**
 * Author: Jingyue
 */

#include "common/InitializePasses.h"
#include "llvm/PassRegistry.h"
using namespace llvm;

struct RegisterIDPasses {
	RegisterIDPasses() {
		PassRegistry &reg = *PassRegistry::getPassRegistry();
		initializeIDAssignerPass(reg);
		initializeIDManagerPass(reg);
		initializeIDTaggerPass(reg);
		initializeValueRenamingPass(reg);
	}
};
static RegisterIDPasses X;
