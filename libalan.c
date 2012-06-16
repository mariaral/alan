#include "symbol.h"
#include "llvm.h"

void init_ready_functions()
{
    SymbolEntry *p;
    /* writeInteger  (n : int)   : proc */
    p = newFunction("writeInteger");
    openScope();
    newParameter("n", typeInteger, PASS_BY_VALUE, p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* writeByte (b : byte)  : proc */
    p = newFunction("writeByte");
    openScope();
    newParameter("b", typeChar, PASS_BY_VALUE,p);
    endFunctionHeader(p,typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* writeChar (b : byte)  : proc */
    p = newFunction("writeChar");
    openScope();
    newParameter("b", typeChar, PASS_BY_VALUE, p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* writeString   (s : reference byte []) : proc */
    p = newFunction("writeString");
    openScope();
    newParameter("s", typeIArray(typeChar), PASS_BY_REFERENCE, p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* readInteger   ()  : int */
    p = newFunction("readInteger");
    openScope();
    endFunctionHeader(p, typeInteger);
    llvm_createFunction(p, true);
    closeScope();

    /* readByte  ()  : byte */
    p = newFunction("readByte");
    openScope();
    endFunctionHeader(p, typeChar);
    llvm_createFunction(p, true);
    closeScope();

    /* readChar  ()  : byte */
    p = newFunction("readChar");
    openScope();
    endFunctionHeader(p, typeChar);
    llvm_createFunction(p, true);
    closeScope();

    /* readString    (n : int, s : reference byte []) : proc */
    p = newFunction("readString");
    openScope();
    newParameter("n",typeInteger,PASS_BY_VALUE,p);
    newParameter("s",typeIArray(typeChar), PASS_BY_REFERENCE, p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* extend (b : byte) : int */
    p = newFunction("extend");
    openScope();
    newParameter("b",typeChar,PASS_BY_VALUE,p);
    endFunctionHeader(p, typeInteger);
    llvm_createFunction(p, true);
    closeScope();

    /* shrink (i : int) : byte */
    p = newFunction("shrink");
    openScope();
    newParameter("i",typeInteger,PASS_BY_VALUE,p);
    endFunctionHeader(p, typeChar);
    llvm_createFunction(p, true);
    closeScope();

    /* strlen (s : reference byte [])    : int */
    p = newFunction("strlen");
    openScope();
    newParameter("s",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    endFunctionHeader(p, typeInteger);
    llvm_createFunction(p, true);
    closeScope();

    /* strcmp (s1 : reference byte [], s2 : reference byte [])   : int */
    p = newFunction("strcmp");
    openScope();
    newParameter("s1",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    newParameter("s2",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    endFunctionHeader(p, typeInteger);
    llvm_createFunction(p, true);
    closeScope();

    /* strcpy (trg : reference byte [], src : reference byte []) : proc */
    p = newFunction("strcpy");
    openScope();
    newParameter("trg",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    newParameter("src",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();

    /* strcat (trg : reference byte [], src : reference byte []) : proc */
    p = newFunction("strcat");
    openScope();
    newParameter("trg",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    newParameter("src",typeIArray(typeChar),PASS_BY_REFERENCE,p);
    endFunctionHeader(p, typeVoid);
    llvm_createFunction(p, true);
    closeScope();
}
