/**
 * Author: Jingyue
 */

#include "llvm/PassRegistry.h"
#include "common/InitializePasses.h"
using namespace llvm;

struct RegisterCFGPasses {
	RegisterCFGPasses() {
		PassRegistry &reg = *PassRegistry::getPassRegistry();
		initializeFPCallGraphPass(reg);
		initializeExecOncePass(reg);
		initializeExecPass(reg);
		initializeICFGBuilderPass(reg);
		initializeIdentifyThreadFuncsPass(reg);
		initializeReachabilityPass(reg);
		initializeIntraReachPass(reg);
		initializePartialICFGBuilderPass(reg);
		initializeMicroBasicBlockBuilderPass(reg);
	}
};
static RegisterCFGPasses X;
