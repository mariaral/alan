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

static struct func_call_tag {
    SymbolEntry *fun_entry;
    LLVMValueRef *call_fac_args;
    SymbolEntry *arg;
    int counter;
    struct func_call_tag *prev;
} *func_call = NULL;

int getNumberOfArgs(SymbolEntry *firstArgument);
LLVMTypeRef convertToLlvmType(Type type, bool byRef);
LLVMValueRef getLlvmLValue(SymbolEntry *rvalEntry);
LLVMValueRef getLlvmRValue(SymbolEntry *rvalEntry, bool byRef);


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
    LLVMAppendBasicBlock(funEntry->u.eFunction.value, "Entry");
}

void llvm_startFunction(SymbolEntry *funEntry)
{
    int numOfArgs, i;
    char *argName;
    SymbolEntry *tempEntry;
    EntriesArray *tempArray;
    LLVMValueRef argValue, allocValue;
    LLVMTypeRef *fac_args;
    LLVMBasicBlockRef block;
    LLVMValueRef func;

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
    LLVMBasicBlockRef block;

    block = LLVMGetLastBasicBlock(funEntry->u.eFunction.value);
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
    LLVMBasicBlockRef block;

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
    varValue = getLlvmLValue(varEntry);

    /* The temporary variable that will be a pointer */
    if(ptrEntry->entryType != ENTRY_TEMPORARY)
        internal("in llvm_createVariable: ptrEntry is not an ENTRY_TEMPORARY\n");

    /* The offset value */
    offsetValue[0] = getLlvmRValue(offsetEntry, false);

    ptrValue = LLVMBuildGEP(builder, varValue, offsetValue, 1, "");
    /* Save value as ptr */
    ptrEntry->u.eTemporary.value = ptrValue;
}

LLVMValueRef getLlvmLValue(SymbolEntry *rvalEntry)
{
    LLVMValueRef lval;

    switch(rvalEntry->entryType) {
    case ENTRY_VARIABLE:
        lval = rvalEntry->u.eVariable.value;
        break;
    case ENTRY_PARAMETER:
        lval = rvalEntry->u.eParameter.value;
        break;
    case ENTRY_TEMPORARY:
        lval = rvalEntry->u.eTemporary.value;
        break;
    default:
        internal("in getLlvmLValue: unsupported entryType for rvalue\n");
        lval = NULL; /* to suppress warnings */
    }

    return lval;
}

LLVMValueRef getLlvmRValue(SymbolEntry *rvalEntry, bool byRef)
{
    LLVMValueRef rval, offsetValue[2];

    switch(rvalEntry->entryType) {
    case ENTRY_VARIABLE:
        if(byRef)
            rval = rvalEntry->u.eVariable.value;
        else
            rval = LLVMBuildLoad(builder, rvalEntry->u.eVariable.value, "");
        break;
    case ENTRY_CONSTANT:
        switch(rvalEntry->u.eConstant.type->kind) {
        case TYPE_INTEGER:
            rval = LLVMConstInt(LLVMInt32Type(), rvalEntry->u.eConstant.value.vInteger, 0);
            break;
        case TYPE_CHAR:
            rval = LLVMConstInt(LLVMInt8Type(), rvalEntry->u.eConstant.value.vChar, 0);
            break;
        case TYPE_ARRAY:
            /* only string can be constant of type array */
            rval = LLVMBuildGlobalString(builder, rvalEntry->u.eConstant.value.vString, "");
            offsetValue[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
            offsetValue[1] = LLVMConstInt(LLVMInt32Type(), 0, 0);
            rval = LLVMBuildGEP(builder, rval, offsetValue, 2, "");
            break;
        default:
            internal("in getLlvmRValue: not a valid constant type\n");
            rval = NULL; /* to suppress warnings */
        }
        break;
    case ENTRY_PARAMETER:
        if(byRef)
            rval = rvalEntry->u.eParameter.value;
        else
            rval = LLVMBuildLoad(builder, rvalEntry->u.eParameter.value, "");
        break;
    case ENTRY_TEMPORARY:
        rval = rvalEntry->u.eTemporary.value;
        if((LLVMGetTypeKind(LLVMTypeOf(rval)) == LLVMPointerTypeKind) && !byRef)
            rval = LLVMBuildLoad(builder, rval, "");
        break;
    default:
        internal("in getLlvmRValue: not a valid entry type\n");
        rval = NULL; /* to suppress warnings */
    }

    return rval;
}


/* ---------------------------------------
 * Operate on function calls
 */
void llvm_createCall(SymbolEntry *funEntry)
{
    int total_args;
    struct func_call_tag *temp;

    if(funEntry->entryType != ENTRY_FUNCTION)
        internal("in llvm_createCall: funEntry is not a function entry\n");

    temp = (struct func_call_tag *) new(sizeof(struct func_call_tag));
    temp->prev = func_call;
    func_call = temp;
    func_call->fun_entry = funEntry;
    total_args = funEntry->u.eFunction.numOfArgs + funEntry->u.eFunction.numOfLifted;
    func_call->call_fac_args = (LLVMValueRef *) new(total_args*sizeof(LLVMValueRef));
    func_call->arg = funEntry->u.eFunction.firstArgument;
    func_call->counter = 0;
}

void llvm_addCallParam(SymbolEntry *parEntry)
{
    int counter;
    SymbolEntry *arg;
    LLVMValueRef tempValue;

    counter = func_call->counter;
    arg = func_call->arg;
    tempValue = getLlvmRValue(parEntry, arg->u.eParameter.mode==PASS_BY_REFERENCE);

    func_call->call_fac_args[counter] = tempValue;
    func_call->arg = arg->u.eParameter.next;
    func_call->counter++;
}

void llvm_doCall(SymbolEntry *result)
{
    SymbolEntry *func = func_call->fun_entry;
    EntriesArray *lifted = func->u.eFunction.liftedArguments;
    LLVMValueRef *call_fac_args = func_call->call_fac_args;
    int counter = func_call->counter;
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

    func_call = func_call->prev;
}


/* ---------------------------------------
 * Operate on statements
 */
void llvm_stmtAssign(SymbolEntry *lvalEntry, SymbolEntry *rvalEntry)
{
    LLVMValueRef lval, rval;

    /* Get the pointer to our lvalue */
    lval = getLlvmLValue(lvalEntry);

    /* Get the value of our rvalue */
    rval = getLlvmRValue(rvalEntry, false);

    /* Store it */
    LLVMBuildStore(builder, rval, lval);
}

void llvm_stmtReturn(SymbolEntry *retEntry)
{
    LLVMValueRef retValue, funcValue;
    LLVMBasicBlockRef block;

    if(retEntry != NULL) {
        retValue = getLlvmRValue(retEntry, false);
        LLVMBuildRet(builder, retValue);
    } else {
        LLVMBuildRetVoid(builder);
    }
    funcValue = currentScope->parent->entries->u.eFunction.value;
    block = LLVMAppendBasicBlock(funcValue, "Unreachable");
    LLVMPositionBuilderAtEnd(builder, block);
}


/* ---------------------------------------
 * Operate on expressions
 */
void llvm_createExpr(llvm_oper loper, SymbolEntry *leftEntry,
        SymbolEntry *rightEntry, SymbolEntry *resultEntry)
{
    LLVMValueRef leftValue, rightValue;

    if(leftEntry != NULL)
        leftValue = getLlvmRValue(leftEntry, false);
    if(rightEntry != NULL)
        rightValue = getLlvmRValue(rightEntry, false);

    switch(loper) {
    case LLVM_CONSTNUM:
        break;
    case LLVM_CONSTCHAR:
        break;
    case LLVM_UPLUS:
        break;
    case LLVM_UMINUS:
        resultEntry->u.eTemporary.value =
            LLVMBuildNeg(builder, rightValue, "");
        break;
    case LLVM_PLUS:
        resultEntry->u.eTemporary.value =
            LLVMBuildAdd(builder, leftValue, rightValue, "");
        break;
    case LLVM_MINUS:
        resultEntry->u.eTemporary.value =
            LLVMBuildSub(builder, leftValue, rightValue, "");
        break;
    case LLVM_MULT:
        resultEntry->u.eTemporary.value =
            LLVMBuildMul(builder, leftValue, rightValue, "");
        break;
    case LLVM_DIV:
        resultEntry->u.eTemporary.value =
            LLVMBuildSDiv(builder, leftValue, rightValue, "");
        break;
    case LLVM_MOD:
        resultEntry->u.eTemporary.value =
            LLVMBuildSRem(builder, leftValue, rightValue, "");
        break;
    }
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
