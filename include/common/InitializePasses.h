/**
 * Author: Jingyue
 */

#ifndef __COMMON_INITIALIZE_PASSES_H
#define __COMMON_INITIALIZE_PASSES_H

namespace llvm {
class PassRegistry;

void initializeCallGraphFPPass(PassRegistry &);
void initializeExecOncePass(PassRegistry &);
void initializeExecPass(PassRegistry &);
void initializeICFGBuilderPass(PassRegistry &);
void initializeIdentifyThreadFuncsPass(PassRegistry &);
void initializeReachabilityPass(PassRegistry &);
void initializeIntraReachPass(PassRegistry &);
void initializePartialICFGBuilderPass(PassRegistry &);
void initializeIDAssignerPass(PassRegistry &);
void initializeIDManagerPass(PassRegistry &);
void initializeIDTaggerPass(PassRegistry &);
void initializeValueRenamingPass(PassRegistry &);
void initializeMicroBasicBlockBuilderPass(PassRegistry &);
void initializeSourceLocatorPass(PassRegistry &);
void initializeFPInstrumenterPass(PassRegistry &);
void initializeFPCollectorPass(PassRegistry &);
void initializePointerAnalysisAnalysisGroup(PassRegistry &);
void initializeBasicPointerAnalysisPass(PassRegistry &);
}

#endif
