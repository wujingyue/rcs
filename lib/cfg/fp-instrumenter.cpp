/**
 * Author: Jingyue
 */

#include <string>
using namespace std;

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "common/InitializePasses.h"
using namespace llvm;

namespace rcs {
	struct FPInstrumenter: public ModulePass {
		static const string TRACE_INIT, TRACE_SOURCE, TRACE_DEST;

		static char ID;

		FPInstrumenter();
		bool runOnModule(Module &M);

	private:
		Function *trace_source, *trace_dest, *trace_init;
	};
}
using namespace rcs;

INITIALIZE_PASS(FPInstrumenter, "instrument-fp",
		"Instrument function pointers", false, false)

char FPInstrumenter::ID = 0;

const string FPInstrumenter::TRACE_INIT = "trace_init";
const string FPInstrumenter::TRACE_SOURCE = "trace_source";
const string FPInstrumenter::TRACE_DEST = "trace_dest";

FPInstrumenter::FPInstrumenter(): ModulePass(ID) {
	initializeFPInstrumenterPass(*PassRegistry::getPassRegistry());
	trace_init = trace_source = trace_dest = NULL;
}

bool FPInstrumenter::runOnModule(Module &M) {
	Function *main = M.getFunction("main");
	assert(main && "Cannot find the main function");
	assert(!M.getFunction(TRACE_INIT));
	assert(!M.getFunction(TRACE_SOURCE));
	assert(!M.getFunction(TRACE_DEST));
	
	const Type *char_ty = IntegerType::get(M.getContext(), 8);
	const Type *int_ty = IntegerType::get(M.getContext(), 32);
	const Type *string_ty = PointerType::getUnqual(char_ty);
	const FunctionType *trace_init_fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			false);
	const FunctionType *trace_dest_fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			vector<const Type *>(1, string_ty),
			false);

	trace_init = dyn_cast<Function>(
			M.getOrInsertFunction(TRACE_INIT, trace_init_fty));
	assert(trace_init);
	trace_dest = dyn_cast<Function>(
			M.getOrInsertFunction(TRACE_DEST, trace_dest_fty));
	assert(trace_dest);

	CallInst::Create(trace_init, "", main->begin()->getFirstNonPHI());
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
		Value *arg = GetElementPtrInst::CreateInBounds(gv,
				indices.begin(), indices.end(), "", insert_pos);
		CallInst::Create(trace_dest, arg, "", insert_pos);
	}

	return true;
}
