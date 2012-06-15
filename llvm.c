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

static LLVMModuleRef mod;
static LLVMBuilderRef builder;
static LLVMValueRef func;
static LLVMBasicBlockRef block;

static struct {
    SymbolEntry *fun_entry;
    LLVMValueRef *call_fac_args;
    SymbolEntry *arg;
    int counter;
} func_call;

int getNumberOfArgs(SymbolEntry *firstArgument);
LLVMTypeRef convertToLlvmType(Type type, bool byRef);


/* ---------------------------------------
 * Operate on module
 */
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


/* ---------------------------------------
 * Operate on functions
 */
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
    funEntry->u.eFunction.type = funcType;
    LLVMSetLinkage(funEntry->u.eFunction.value, LLVMInternalLinkage);
    /* And create the entry basic block */
    LLVMAppendBasicBlock(funEntry->u.eFunction.value, "entry");
}

void llvm_startFunction(SymbolEntry *funEntry)
{
    int numOfArgs, i;
    char *argName;
    SymbolEntry *tempEntry;
    EntriesArray *tempArray;
    LLVMValueRef argValue, allocValue;
    LLVMTypeRef *fac_args;


    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("llvm_startFunction called without a function entry\n");

    func = funEntry->u.eFunction.value;
    block = LLVMGetFirstBasicBlock(func);
    LLVMPositionBuilderAtEnd(builder, block);

    /* Get the type of the arguments */
    numOfArgs = funEntry->u.eFunction.numOfArgs + funEntry->u.eFunction.numOfLifted;
    fac_args = (LLVMTypeRef *) new(numOfArgs * sizeof(LLVMTypeRef));
    LLVMGetParamTypes(funEntry->u.eFunction.type, fac_args);

    /* Allocate stack memory for arguments */
    tempEntry = funEntry->u.eFunction.firstArgument;
    i = 0;
    while(tempEntry != NULL) {
        argValue = LLVMGetParam(func, i);
        if(tempEntry->u.eParameter.mode == PASS_BY_VALUE) {
            argName = getEntryName(tempEntry);
            allocValue = LLVMBuildAlloca(builder, fac_args[i], argName);
            LLVMBuildStore(builder, argValue, allocValue);
            tempEntry->u.eParameter.value = allocValue;
        } else {
            tempEntry->u.eParameter.value = argValue;
        }
        tempEntry = tempEntry->u.eParameter.next;
        i++;
    }
    /* And save lifted arguments as well */
    tempArray = funEntry->u.eFunction.liftedArguments;
    while(tempArray != NULL) {
        argValue = LLVMGetParam(func, i++);
        tempEntry = tempArray->entry;
        switch(tempEntry->entryType) {
        case ENTRY_VARIABLE:
            tempEntry->u.eVariable.value = argValue;
            break;
        case ENTRY_PARAMETER:
            tempEntry->u.eParameter.value = argValue;
            break;
        default:
            internal("lifted variable has to be ENTRY_VARIABLE of ENTRY_PARAMETER\n");
        }
        tempArray = tempArray->next;
    }

    /* Now restore my local variables */
    tempEntry = currentScope->entries;
    while(tempEntry != NULL) {
        if(tempEntry->entryType == ENTRY_VARIABLE)
            tempEntry->u.eVariable.value = tempEntry->u.eVariable.temp_value;
        tempEntry = tempEntry->nextInScope;
    }
}

void llvm_closeFunction(SymbolEntry *funEntry)
{
    LLVMValueRef instr;
    LLVMOpcode op;
    LLVMTypeRef retType;

    instr = LLVMGetLastInstruction(block);
    if(!instr ||
      ((op=LLVMGetInstructionOpcode(instr))!=LLVMRet && op!=LLVMBr)) {
        /* we have to return something */
        retType = LLVMGetReturnType(funEntry->u.eFunction.type);
        LLVMBuildRet(builder, LLVMConstNull(retType));
    }
}


/* ---------------------------------------
 * Operate on variables
 */
void llvm_createVariable(SymbolEntry *varEntry)
{
    char *name;
    SymbolEntry *funEntry;
    LLVMValueRef varValue;
    LLVMTypeRef varType, elemType;
    unsigned arrayLen;
    LLVMValueRef vArrayLen;

    if(varEntry->entryType != ENTRY_VARIABLE)
        internal("in llvm_createVariable: varEntry is not an ENTRY_VARIABLE\n");
    name = getEntryName(varEntry);
    varType = convertToLlvmType(varEntry->u.eVariable.type, false);

    /* Get the basic block of our function */
    funEntry = currentScope->parent->entries;
    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("in llvm_createVariable: parent entry supposed to be function\n");
    block = LLVMGetFirstBasicBlock(funEntry->u.eFunction.value);
    LLVMPositionBuilderAtEnd(builder, block);

    /* And allocate stack memory for our variable */
    switch(LLVMGetTypeKind(varType)) {
    case LLVMIntegerTypeKind:
        varValue = LLVMBuildAlloca(builder, varType, name);
        break;
    case LLVMArrayTypeKind:
        elemType = LLVMGetElementType(varType);
        arrayLen = LLVMGetArrayLength(varType);
        vArrayLen = LLVMConstInt(LLVMInt32Type(), arrayLen, 0);
        varValue = LLVMBuildArrayAlloca(builder, elemType, vArrayLen, name);
        break;
    default:
        internal("in llvm_createVariable: not a valid variable type\n");
        varValue = NULL; /* to suppress warnings */
    }

    varEntry->u.eVariable.temp_value = varValue;
}

void llvm_arrayValue(SymbolEntry *varEntry,
        SymbolEntry *ptrEntry, SymbolEntry *offsetEntry)
{
    LLVMValueRef varValue, ptrValue, offsetValue[1];

    /* The array variable */
    switch(varEntry->entryType) {
    case ENTRY_VARIABLE:
        varValue = varEntry->u.eVariable.value;
        break;
    case ENTRY_PARAMETER:
        varValue = varEntry->u.eParameter.value;
        break;
    default:
        internal("in llvm_arrayVariable: unsupported entryType for varEntry\n");
        varValue = NULL; /* to suppress warnings */
    }

    /* The temporary variable that will be a pointer */
    if(ptrEntry->entryType != ENTRY_TEMPORARY)
        internal("in llvm_createVariable: ptrEntry is not an ENTRY_TEMPORARY\n");

    /* The offset value */
    switch(offsetEntry->entryType) {
    case ENTRY_VARIABLE:
        offsetValue[0] = LLVMBuildLoad(builder, offsetEntry->u.eVariable.value, "");
        break;
    case ENTRY_CONSTANT:
        offsetValue[0] = LLVMConstInt(LLVMInt32Type(), offsetEntry->u.eConstant.value.vInteger, 0);
        break;
    case ENTRY_PARAMETER:
        offsetValue[0] = LLVMBuildLoad(builder, offsetEntry->u.eParameter.value, "");
        break;
    case ENTRY_TEMPORARY:
        offsetValue[0] = offsetEntry->u.eTemporary.value;
        if(LLVMGetTypeKind(LLVMTypeOf(offsetValue[0])) == LLVMPointerTypeKind)
            offsetValue[0] = LLVMBuildLoad(builder, offsetValue[0], "");
        break;
    default:
        internal("in llvm_arrayVariable: unsupported entryType for offsetEntry\n");
        offsetValue[0] = NULL; /* to suppress warnings */
    }

    ptrValue = LLVMBuildGEP(builder, varValue, offsetValue, 1, "");
    /* Save value as ptr */
    ptrEntry->u.eTemporary.value = ptrValue;
}


/* ---------------------------------------
 * Operate on function calls
 */
void llvm_createCall(SymbolEntry *funEntry)
{
    int total_args;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("in llvm_createCall: funEntry is not a function entry\n");

    func_call.fun_entry = funEntry;
    total_args = funEntry->u.eFunction.numOfArgs + funEntry->u.eFunction.numOfLifted;
    func_call.call_fac_args = (LLVMValueRef *) new(total_args*sizeof(LLVMValueRef));
    func_call.arg = funEntry->u.eFunction.firstArgument;
    func_call.counter = 0;
}

/* XXX: handle nested calls */
void llvm_addCallParam(SymbolEntry *parEntry)
{
    int counter;
    SymbolEntry *arg;
    LLVMValueRef tempValue;
    LLVMValueRef offsetValue[2];

    counter = func_call.counter;
    arg = func_call.arg;
    switch(parEntry->entryType) {
    case ENTRY_VARIABLE:
        if(arg->u.eParameter.mode == PASS_BY_VALUE)
            tempValue = LLVMBuildLoad(builder, parEntry->u.eVariable.value, "");
        else
            tempValue = parEntry->u.eVariable.value;
        break;
    case ENTRY_CONSTANT:
        switch(parEntry->u.eConstant.type->kind) {
        case TYPE_INTEGER:
            tempValue = LLVMConstInt(LLVMInt32Type(), parEntry->u.eConstant.value.vInteger, 0);
            break;
        case TYPE_CHAR:
            tempValue = LLVMConstInt(LLVMInt8Type(), parEntry->u.eConstant.value.vChar, 0);
            break;
        case TYPE_ARRAY:
            /* only string can be constant of type array */
            tempValue = LLVMBuildGlobalString(builder, parEntry->u.eConstant.value.vString, "");
            offsetValue[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
            offsetValue[1] = LLVMConstInt(LLVMInt32Type(), 0, 0);
            tempValue = LLVMBuildGEP(builder, tempValue, offsetValue, 2, "");
            break;
        default:
            internal("in llvm_addCallParam: not a valid constant type\n");
            tempValue = NULL; /* to suppress warnings */
        }
        break;
    case ENTRY_PARAMETER:
        if(arg->u.eParameter.mode == PASS_BY_VALUE)
            tempValue = LLVMBuildLoad(builder, parEntry->u.eParameter.value, "");
        else
            tempValue = parEntry->u.eParameter.value;
        break;
    case ENTRY_TEMPORARY:
        tempValue = parEntry->u.eTemporary.value;
        if((LLVMGetTypeKind(LLVMTypeOf(tempValue)) == LLVMPointerTypeKind)
        && (arg->u.eParameter.mode == PASS_BY_VALUE))
            tempValue = LLVMBuildLoad(builder, tempValue, "");
        break;
    default:
        internal("in llvm_addCallParam: not a valid entry type\n");
        tempValue = NULL; /* to suppress warnings */
    }

    func_call.call_fac_args[counter] = tempValue;
    func_call.arg = arg->u.eParameter.next;
    func_call.counter++;
}

void llvm_doCall(SymbolEntry *result)
{
    SymbolEntry *func = func_call.fun_entry;
    EntriesArray *lifted = func->u.eFunction.liftedArguments;
    LLVMValueRef *call_fac_args = func_call.call_fac_args;
    int counter = func_call.counter;
    SymbolEntry *temp;
    LLVMValueRef fun_value = func->u.eFunction.value;

    /* Pass the lifted values as arguments */
    while(lifted != NULL) {
        temp = lifted->entry;
        switch(temp->entryType) {
        case ENTRY_VARIABLE:
            call_fac_args[counter] = temp->u.eVariable.value;
            break;
        case ENTRY_PARAMETER:
            call_fac_args[counter] = temp->u.eParameter.value;
            break;
        default:
            internal("in llvm_doCall: not a valid lifted entry type\n");
        }
        counter++;
        lifted = lifted->next;
    }

    /* result is always temp entry */
    fun_value =
        LLVMBuildCall(builder, fun_value, call_fac_args, counter, "");

    if(result)
        result->u.eTemporary.value = fun_value;
}


/* ---------------------------------------
 * Operate on statements
 */
void llvm_stmtAssign(SymbolEntry *lvalEntry, SymbolEntry *rvalEntry)
{
}


/* ---------------------------------------
 * Operate on Types
 */
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
        if(byRef)
            return LLVMPointerType(temp, 0);
        else
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
    return NULL;
}
