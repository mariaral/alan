/******************************************************************************
 *  CVS version:
 *     $Id: general.c,v 1.1 2004/05/05 22:00:08 nickie Exp $
 ******************************************************************************
 *
 *  C code file : general.c
 *  Project     : PCL Compiler
 *  Version     : 1.0 alpha
 *  Written by  : Nikolaos S. Papaspyrou (nickie@softlab.ntua.gr)
 *  Date        : May 5, 2004
 *  Description : Generic symbol table in C, general variables and functions
 *
 *  Comments: (in Greek iso-8859-7)
 *  ---------
 *  Εθνικό Μετσόβιο Πολυτεχνείο.
 *  Σχολή Ηλεκτρολόγων Μηχανικών και Μηχανικών Υπολογιστών.
 *  Τομέας Τεχνολογίας Πληροφορικής και Υπολογιστών.
 *  Εργαστήριο Τεχνολογίας Λογισμικού
 */


/* ---------------------------------------------------------------------
   ---------------------------- Header files ---------------------------
   --------------------------------------------------------------------- */

#include <stdlib.h>

#include "general.h"
#include "symbol.h"
#include "error.h"


/* ---------------------------------------------------------------------
   ----------- Υλοποίηση των συναρτήσεων διαχείρισης μνήμης ------------
   --------------------------------------------------------------------- */

void * new (size_t size)
{
   void * result = malloc(size);
   
   if (result == NULL)
      fatal("\rOut of memory");
   return result;
}

void delete (void * p)
{
   if (p != NULL)
      free(p);
}

/* ---------------------------------------------------------------------
   ----------- Δήλωση των έτοιμων συναρτήσεων της γλώσσας --------------
   --------------------------------------------------------------------- */

void init_ready_functions()
{
   SymbolEntry *p;
   /* writeInteger	(n : int)	: proc */
   p = newFunction("writeInteger");
   openScope(); 
   newParameter("n", typeInteger, PASS_BY_VALUE, p);
   endFunctionHeader(p, typeVoid);
   closeScope();

   /* writeByte	(b : byte)	: proc */
   p = newFunction("writeByte");
   openScope();
   newParameter("b", typeChar, PASS_BY_VALUE,p);
   endFunctionHeader(p,typeVoid);
   closeScope();

   /* writeChar	(b : byte)	: proc */
   p = newFunction("writeChar"); 
   openScope();
   newParameter("b", typeChar, PASS_BY_VALUE, p);
   endFunctionHeader(p, typeVoid);
   closeScope();

   /* writeString	(s : reference byte [])	: proc */
   p = newFunction("writeString"); 
   openScope();
   newParameter("s", typeIArray(typeChar), PASS_BY_REFERENCE, p);
   endFunctionHeader(p, typeVoid);
   closeScope();

   /* readInteger	()	: int */
   p = newFunction("readInteger"); 
   openScope();
   endFunctionHeader(p, typeInteger);
   closeScope();

   /* readByte	()	: byte */
   p = newFunction("readByte");
   openScope();
   endFunctionHeader(p, typeChar);
   closeScope();

   /* readChar	()	: byte */
   p = newFunction("readChar");
   openScope();
   endFunctionHeader(p, typeChar);
   closeScope();

   /* readString	(n : int, s : reference byte []) : proc */
   p = newFunction("readString");
   openScope();
   newParameter("n",typeInteger,PASS_BY_VALUE,p);
   newParameter("s",typeIArray(typeChar), PASS_BY_REFERENCE, p);
   endFunctionHeader(p, typeVoid);
   closeScope();

   /* extend (b : byte) : int */
   p = newFunction("extend");
   openScope();
   newParameter("b",typeChar,PASS_BY_VALUE,p);
   endFunctionHeader(p, typeInteger);
   closeScope();
      
   /* shrink (i : int) : byte */
   p = newFunction("shrink");
   openScope();
   newParameter("i",typeInteger,PASS_BY_VALUE,p);
   endFunctionHeader(p, typeChar);
   closeScope();
   
   /* strlen (s : reference byte [])	: int */
   p = newFunction("strlen");
   openScope();
   newParameter("s",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   endFunctionHeader(p, typeInteger);
   closeScope();
   
   /* strcmp (s1 : reference byte [], s2 : reference byte [])   : int */
   p = newFunction("strcmp");
   openScope();
   newParameter("s1",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   newParameter("s2",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   endFunctionHeader(p, typeInteger);
   closeScope();
   
   /* strcpy (trg : reference byte [], src : reference byte []) : proc */
   p = newFunction("strcpy");
   openScope();
   newParameter("trg",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   newParameter("src",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   endFunctionHeader(p, typeVoid);
   closeScope();
   
   /* strcat (trg : reference byte [], src : reference byte []) : proc */
   p = newFunction("strcat");
   openScope();
   newParameter("trg",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   newParameter("src",typeIArray(typeChar),PASS_BY_REFERENCE,p);
   endFunctionHeader(p, typeVoid);
   closeScope();
   
}

/* ---------------------------------------------------------------------
   ------- Αρχείο εισόδου του μεταγλωττιστή και αριθμός γραμμής --------
   --------------------------------------------------------------------- */

const char * filename;
int linecount;
