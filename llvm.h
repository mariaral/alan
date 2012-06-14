#ifndef LLVM_H
#define LLVM_H 0

#include "symbol.h"

/* Operate on module */
void llvm_createModule();
void llvm_destroyModule();
void llvm_printModule(char *filename);

/* Operate on functions */
void llvm_createFunction(SymbolEntry *funEntry);
void llvm_startFunction(SymbolEntry *funEntry);
void llvm_closeFunction(SymbolEntry *funEntry);

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

#endif
