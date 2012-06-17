#ifndef LLVM_H
#define LLVM_H 0

#include <stdbool.h>
#include "symbol.h"

/* Operate on module */
void llvm_createModule();
void llvm_destroyModule();
void llvm_printModule(char *filename);

/* Operate on functions */
void llvm_createFunction(SymbolEntry *funEntry, bool isLib);
void llvm_startFunction(SymbolEntry *funEntry);
void llvm_closeFunction(SymbolEntry *funEntry);
void llvm_createMain(SymbolEntry *funEntry);

/* Operate on variables */
void llvm_createVariable(SymbolEntry *varEntry);
void llvm_arrayValue(SymbolEntry *varEntry,
        SymbolEntry *ptrEntry, SymbolEntry *offsetEntry);

/* Operate on function calls */
void llvm_createCall(SymbolEntry *funEntry);
void llvm_addCallParam(SymbolEntry *parEntry);
void llvm_doCall(SymbolEntry *result);

/* Operate on statements */
void llvm_stmtAssign(SymbolEntry *lvalEntry, SymbolEntry *rvalEntry);
void llvm_stmtReturn(SymbolEntry *retEntry);

/* Operate on expressions */
typedef enum {
    LLVM_CONSTNUM, LLVM_CONSTCHAR, LLVM_UPLUS, LLVM_UMINUS,
    LLVM_PLUS, LLVM_MINUS, LLVM_MULT, LLVM_DIV, LLVM_MOD
} llvm_oper;
void llvm_createExpr(llvm_oper loper, SymbolEntry *leftEntry,
        SymbolEntry *rightEntry, SymbolEntry *resultEntry);

/* Operate on conditional blocks */
typedef enum{ LLVM_COND, LLVM_LOOP } llvm_cond;
void llvm_createBlock(llvm_cond lcond);
void llvm_exitBlock(llvm_cond lcond);
typedef enum { LLVM_TRUEB, LLVM_FALSEB, LLVM_EXITB, LLVM_LOOPB } llvm_block;
void llvm_newBlock(llvm_block lblock);
void llvm_startBlock(llvm_block lblock);
void llvm_jumpBlock(llvm_block lblock);
typedef enum {
    LLVM_EXCL, LLVM_EQ, LLVM_NE, LLVM_QT, LLVM_LT, LLVM_QE, LLVM_LE
} llvm_logic;
void llvm_createLogic(llvm_logic llogic, SymbolEntry *leftEntry, SymbolEntry *rightEntry);

#endif
