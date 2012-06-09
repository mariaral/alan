#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include "llvm.h"
#include "symbol.h"
#include "error.h"
#include "general.h"


extern Scope *currentScope;

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
    char *funName;
    int numOfArgs;
    SymbolEntry *parentFun, *tempEntry;
    EntriesArray *lifted, *tempArray;
    LLVMTypeRef *fac_args;
    LLVMTypeRef resultType;
    LLVMTypeRef funcType;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("llvm_createFunction called without a function entry\n");

    /* First found out all the lifted parameters */
    funEntry->u.eFunction.liftedArguments = NULL;
    funEntry->u.eFunction.numOfLifted = 0;
    if(currentScope->nestingLevel > 2) {
        parentFun = currentScope->parent->parent->entries;
        if(parentFun->entryType != ENTRY_FUNCTION)
            internal("parent entry was supposed to be a function\n");
        /* Take the arguments of my parent */
        tempEntry = parentFun->u.eFunction.firstArgument;
        while(tempEntry != NULL) {
            lifted = (EntriesArray *) new(sizeof(EntriesArray));
            lifted->next = funEntry->u.eFunction.liftedArguments;
            lifted->entry = tempEntry;
            funEntry->u.eFunction.liftedArguments = lifted;
            funEntry->u.eFunction.numOfLifted++;
            tempEntry = tempEntry->u.eParameter.next;
        }
        /* Take the lifted arguments of my parent */
        tempArray = parentFun->u.eFunction.liftedArguments;
        while(tempArray != NULL) {
            lifted = (EntriesArray *) new(sizeof(EntriesArray));
            lifted->next = funEntry->u.eFunction.liftedArguments;
            lifted->entry = tempArray->entry;
            funEntry->u.eFunction.liftedArguments = lifted;
            funEntry->u.eFunction.numOfLifted++;
            tempArray = tempArray->next;
        }
        /* And lift all the variables of my parent */
        tempEntry = currentScope->parent->entries->nextInScope;
        while(tempEntry != NULL) {
            if(tempEntry->entryType != ENTRY_VARIABLE) {
                tempEntry = tempEntry->nextInScope;
                continue;
            }
            lifted = (EntriesArray *) new(sizeof(EntriesArray));
            lifted->next = funEntry->u.eFunction.liftedArguments;
            lifted->entry = tempEntry;
            funEntry->u.eFunction.liftedArguments = lifted;
            funEntry->u.eFunction.numOfLifted++;
            tempEntry = tempEntry->nextInScope;
        }
    }

    /* Get the name of the function */
    funName =  getEntryName(funEntry);
    /* Allocate array for function's parameters */
    numOfArgs = funEntry->u.eFunction.numOfArgs + funEntry->u.eFunction.numOfLifted;
    fac_args = (LLVMTypeRef *) new(numOfArgs * sizeof(LLVMTypeRef));
    /* Pass the real arguments first */
    i = 0;
    tempEntry = funEntry->u.eFunction.firstArgument;
    while(tempEntry != NULL) {
        fac_args[i++] = convertToLlvmType(tempEntry->u.eParameter.type,
                                tempEntry->u.eParameter.mode==PASS_BY_REFERENCE);
        tempEntry = tempEntry->u.eParameter.next;
    }
    /* Then the lifted parameters */
    tempArray = funEntry->u.eFunction.liftedArguments;
    while(tempArray != NULL) {
        tempEntry = tempArray->entry;
        switch(tempEntry->entryType) {
        case ENTRY_VARIABLE:
            fac_args[i++] = convertToLlvmType(tempEntry->u.eVariable.type, true);
            break;
        case ENTRY_PARAMETER:
            fac_args[i++] = convertToLlvmType(tempEntry->u.eParameter.type, true);
            break;
        default:
            internal("lifted variable has to be ENTRY_VARIABLE of ENTRY_PARAMETER\n");
        }
        tempArray = tempArray->next;
    }
    /* Get the function type */
    resultType = convertToLlvmType(funEntry->u.eFunction.resultType, false);
    funcType = LLVMFunctionType(resultType, fac_args, numOfArgs, 0);
    funEntry->u.eFunction.value = LLVMAddFunction(mod, funName, funcType);
    LLVMSetLinkage(funEntry->u.eFunction.value, LLVMInternalLinkage);

    /*
    argEntry = funEntry->u.eFunction.firstArgument;
    for(i=0; i<numOfArgs; i++) {
        argName = getEntryName(argEntry);
        if(argEntry->u.eParameter.mode == PASS_BY_VALUE) {
            argValue = LLVMAddGlobal(mod, fac_args[i], argName);
            LLVMSetInitializer(argValue, LLVMConstNull(fac_args[i]));
            LLVMSetLinkage(argValue, LLVMInternalLinkage);
            argEntry->u.eParameter.value = argValue;
        }
        argEntry = argEntry->u.eParameter.next;
    }
    */
}

void llvm_startFunction(SymbolEntry *funEntry)
{
    /*
    int numOfArgs, i;
    SymbolEntry *argEntry;
    LLVMValueRef argValue;

    LLVMBasicBlockRef entry;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("llvm_startFunction called without a function entry\n");

    func = funEntry->u.eFunction.value;
    entry = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    numOfArgs = funEntry->u.eFunction.numOfArgs;
    argEntry = funEntry->u.eFunction.firstArgument;
    for(i=0; i<numOfArgs; i++) {
        argValue = LLVMGetParam(func, i);
        LLVMBuildStore(builder, argValue, argEntry->u.eParameter.value);
        argEntry = argEntry->u.eParameter.next;
    }
    */
}


/* Operate on variables */
void llvm_createVariable(SymbolEntry *varEntry)
{
    /*
    char *name;
    LLVMValueRef varValue;
    LLVMTypeRef varType;

    varType = convertToLlvmType(varEntry->u.eVariable.type, false);
    name = getEntryName(varEntry);
    varValue = LLVMAddGlobal(mod, varType, name);
    LLVMSetInitializer(varValue, LLVMConstNull(varType));
    LLVMSetLinkage(varValue, LLVMInternalLinkage);
    varEntry->u.eVariable.value = varValue;
    */
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
        temp = convertToLlvmType(type->refType, false);
        return LLVMArrayType(temp, type->size);
        break;
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
