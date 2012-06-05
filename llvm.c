#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include "llvm.h"
#include "symbol.h"
#include "error.h"
#include "general.h"


LLVMModuleRef mod;
LLVMBuilderRef builder;
LLVMValueRef func;

int getNumberOfArgs(SymbolEntry *firstArgument);
LLVMTypeRef convertToLlvmType(Type type, bool byRef);


/* Operate on module */
void llvm_createModule()
{
    mod = LLVMModuleCreateWithName("alan_module");
    builder = LLVMCreateBuilder();
}

void llvm_destroyModule()
{
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(mod);
}

void llvm_printModule(char *filename)
{
    LLVMWriteBitcodeToFile(mod, filename);
}


/* Operate on functions */
void llvm_createFunction(SymbolEntry *funEntry)
{
    int i;
    char *funName, *argName;
    int numOfArgs;
    SymbolEntry *argEntry;
    LLVMTypeRef *fac_args;
    LLVMTypeRef resultType;
    LLVMTypeRef funcType;
    LLVMValueRef argValue;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("llvm_createFunction called without a function entry\n");

    funName =  getEntryName(funEntry);
    argEntry = funEntry->u.eFunction.firstArgument;
    numOfArgs = funEntry->u.eFunction.numOfArgs;
    fac_args = (LLVMTypeRef *) new(numOfArgs * sizeof(LLVMTypeRef));
    for(i=0; i<numOfArgs; i++) {
        fac_args[i] = convertToLlvmType(argEntry->u.eParameter.type,
                                        argEntry->u.eParameter.mode==PASS_BY_REFERENCE);
        argEntry = argEntry->u.eParameter.next;
    }

    resultType = convertToLlvmType(funEntry->u.eFunction.resultType, false);
    funcType = LLVMFunctionType(resultType, fac_args, numOfArgs, 0);
    func = LLVMAddFunction(mod, funName, funcType);
    LLVMSetLinkage(func, LLVMInternalLinkage);

    funEntry->u.eFunction.value = func;

    argEntry = funEntry->u.eFunction.firstArgument;
    for(i=0; i<numOfArgs; i++) {
        if(argEntry->u.eParameter.mode==PASS_BY_VALUE) {
            argName = getEntryName(argEntry);
            argValue = LLVMAddGlobal(mod, fac_args[i], argName);
            LLVMSetLinkage(argValue, LLVMInternalLinkage);
        } else {
            argValue = LLVMGetParam(func, i);
        }
        argEntry->u.eParameter.value = argValue;
        argEntry = argEntry->u.eParameter.next;
    }
}

void llvm_startFunction(SymbolEntry *funEntry)
{
    LLVMBasicBlockRef entry;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("llvm_startFunction called without a function entry\n");

    func = funEntry->u.eFunction.value;
    entry = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
}


/* Operate on Types */
LLVMTypeRef convertToLlvmType(Type type, bool byRef)
{
    LLVMTypeRef temp;

    switch(type->kind) {
    case TYPE_VOID:
        return LLVMVoidType();
        break;
    case TYPE_INTEGER:
        if(byRef)
            return LLVMPointerType(LLVMInt32Type(), 0);
        else
            return LLVMInt32Type();
        break;
    case TYPE_BOOLEAN:
        internal("Alan does not support TYPE_BOOLEAN\n");
        break;
    case TYPE_CHAR:
        if(byRef)
            return LLVMPointerType(LLVMInt8Type(), 0);
        else
            return LLVMInt8Type();
        break;
    case TYPE_REAL:
        internal("Alan does not support TYPE_REAL\n");
        break;
    case TYPE_ARRAY:
    case TYPE_IARRAY:
    case TYPE_POINTER:
        temp = convertToLlvmType(type->refType, false);
        return LLVMPointerType(temp, 0);
        break;
    case TYPE_UNKNOWN:
        internal("Type unknown??\n");
        break;
    }

    /* Just to suspend warnings */
    return LLVMInt32Type();
}
