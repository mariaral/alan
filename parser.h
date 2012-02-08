
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_eof = 0,
     T_byte = 258,
     T_int = 259,
     T_proc = 260,
     T_reference = 261,
     T_return = 262,
     T_while = 263,
     T_if = 264,
     T_else = 265,
     T_true = 266,
     T_false = 267,
     T_id = 268,
     T_string = 269,
     T_constnum = 270,
     T_constchar = 271,
     T_assign = 272,
     T_mod = 273,
     T_excl = 274,
     T_and = 275,
     T_or = 276,
     T_eq = 277,
     T_ne = 278,
     T_lt = 279,
     T_le = 280,
     T_gt = 281,
     T_ge = 282,
     T_oppar = 283,
     T_clpar = 284,
     T_opj = 285,
     T_clj = 286,
     T_begin = 287,
     T_end = 288,
     T_dd = 289,
     T_comma = 290,
     T_semic = 291,
     T_newline = 292,
     UMINUS = 293,
     UPLUS = 294
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 23 "parser.y"

        int     i;
        char    c;
        Type type;
        char s[32];
        struct {
                SymbolEntry *Place;
                Type type;
        } var;
        labelList *Next;
        struct {
                labelList *True;
                labelList *False;
        } b;
        char Name[256];



/* Line 1676 of yacc.c  */
#line 111 "parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


