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

#endif
