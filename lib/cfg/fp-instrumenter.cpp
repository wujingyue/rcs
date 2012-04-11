/**
 * Author: Jingyue
 */

#include <string>
using namespace std;

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include "common/InitializePasses.h"
using namespace llvm;

#include "common/IDManager.h"
using namespace rcs;

namespace rcs {
	struct FPInstrumenter: public ModulePass {
		static const string TRACE_INIT, TRACE_SOURCE, TRACE_DEST;

		static char ID;

		FPInstrumenter();
		bool runOnModule(Module &M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const;

	private:
		Function *trace_source, *trace_dest, *trace_init;
	};
}

INITIALIZE_PASS_BEGIN(FPInstrumenter, "instrument-fp",
		"Instrument function pointers", false, false)
INITIALIZE_PASS_DEPENDENCY(IDManager)
INITIALIZE_PASS_END(FPInstrumenter, "instrument-fp",
		"Instrument function pointers", false, false)

void FPInstrumenter::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<IDManager>();
}

char FPInstrumenter::ID = 0;

const string FPInstrumenter::TRACE_INIT = "trace_init";
const string FPInstrumenter::TRACE_SOURCE = "trace_source";
const string FPInstrumenter::TRACE_DEST = "trace_dest";

FPInstrumenter::FPInstrumenter(): ModulePass(ID) {
	initializeFPInstrumenterPass(*PassRegistry::getPassRegistry());
	trace_init = trace_source = trace_dest = NULL;
}

bool FPInstrumenter::runOnModule(Module &M) {
	IDManager &IDM = getAnalysis<IDManager>();

	Function *main = M.getFunction("main");
	assert(main && "Cannot find the main function");
	assert(!M.getFunction(TRACE_INIT) && "trace_init already exists");
	assert(!M.getFunction(TRACE_SOURCE) && "trace_source already exists");
	assert(!M.getFunction(TRACE_DEST) && "trace_dest already exists");
	
	Type *char_ty = IntegerType::get(M.getContext(), 8);
	Type *int_ty = IntegerType::get(M.getContext(), 32);
	Type *string_ty = PointerType::getUnqual(char_ty);
	// trace_init()
	FunctionType *trace_init_fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			false);
	// trace_source(int)
	FunctionType *trace_source_fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			vector<Type *>(1, int_ty),
			false);
	// trace_dest(char *)
	FunctionType *trace_dest_fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			vector<Type *>(1, string_ty),
			false);

	trace_init = Function::Create(trace_init_fty,
			GlobalValue::ExternalLinkage, TRACE_INIT, &M);
	trace_source = Function::Create(trace_source_fty,
			GlobalValue::ExternalLinkage, TRACE_SOURCE, &M);
	trace_dest = Function::Create(trace_dest_fty,
			GlobalValue::ExternalLinkage, TRACE_DEST, &M);

	// Insert trace_source at each call via a function pointer.
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		for (Function::iterator bb = f->begin(); bb != f->end(); ++bb) {
			for (BasicBlock::iterator ins = bb->begin(); ins != bb->end(); ++ins) {
				CallSite cs(ins);
				if (cs.getInstruction() && cs.getCalledFunction() == NULL) {
					unsigned ins_id = IDM.getInstructionID(ins);
					assert(ins_id != IDManager::INVALID_ID);
					Value *arg = ConstantInt::get(int_ty, ins_id);
					CallInst::Create(trace_source, arg, "", ins);
				}
			}
		}
	}

	// Insert trace_dest at each function entry. 
	for (Module::iterator f = M.begin(); f != M.end(); ++f) {
		if (f->isDeclaration())
			continue;

		// Construct a global array representing the function name.
		string function_name = f->getName();
		vector<Constant *> bytes;
		for (size_t i = 0; i < function_name.size(); ++i)
			bytes.push_back(ConstantInt::get(char_ty, function_name[i]));
		bytes.push_back(ConstantInt::get(char_ty, 0));
		Constant *bytes_arr = ConstantArray::get(
				ArrayType::get(char_ty, bytes.size()),
				bytes);
		GlobalVariable *gv = new GlobalVariable(
				M, // Containing module
				bytes_arr->getType(), // Type
				true, // Is constant?
				GlobalValue::PrivateLinkage, // Linkage
				bytes_arr, // Initializer
				".func_name" // Suggested name
				);
		
		// Insert a trace_dest function call at the function entry. 
		vector<Value *> indices(2, ConstantInt::get(int_ty, 0));
		Instruction *insert_pos = f->begin()->getFirstNonPHI();
		Value *arg = GetElementPtrInst::CreateInBounds(gv, indices, "", insert_pos);
		CallInst::Create(trace_dest, arg, "", insert_pos);
	}

	// Insert trace_init at the program entry. 
	// Do this after inserting trace_dest. 
	CallInst::Create(trace_init, "", main->begin()->getFirstNonPHI());

	return true;
}
