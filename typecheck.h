#ifndef TYPECHECK_H
#define TYPECHECK_H 0

#include "quad.h"

bool int_or_byte(Type, Type);

bool equalArrays(Type, Type);

Type paramType(SymbolEntry*);

bool paramChecked(bool*, SymbolEntry**, varstr);

bool paramString(bool*, SymbolEntry**);

#endif

