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

#endif
