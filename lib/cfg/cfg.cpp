/**
 * Author: Jingyue
 */

#include "common/InitializePasses.h"
#include "llvm/PassRegistry.h"
using namespace llvm;

struct RegisterCFGPasses {
	RegisterCFGPasses() {
		PassRegistry &reg = *PassRegistry::getPassRegistry();
		initializeCallGraphFPPass(reg);
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
