%{
/*
****************************************************
             LEXICAL ANALYSER-FLEX FILE
****************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol.h"
#include "quad.h"
#include "parser.h"



extern int linecount;
int commentcount=0;

int installNum(void);
%}



white  [ \t]
letter [A-Za-z]
digit  [0-9]
hex [A-Fa-f]|{digit}
simplechar [^\x00-\x1F\\\'\"]
specialchar \\([ntr0\\\"\']|x{hex}{2})

%x COMMENT

%%

{white}*	{ /* nothing */ }
<*>\n		{ linecount++;}

"byte"		{ return T_byte; }
"else"		{ return T_else; }
"false"		{ return T_false; }
"if"		{ return T_if; }
"int"		{ return T_int; }
"proc"		{ return T_proc; }
"reference"	{ return T_reference; }
"return"	{ return T_return; }
"while"		{ return T_while; }
"true"		{ return T_true; }


{digit}+	{yylval.i = (int)installNum(); return(T_constnum);}
{letter}({letter}|_|{digit})*		{ strncpy(yylval.s,yytext,32); return T_id; }
\'({simplechar}|{specialchar})\'	{ /*yylval.Name = (char *)strdup(yytext);*/ return T_constchar; }
\"({simplechar}|{white}|\\)*\"		{/*yylval.Name = (char *)strdup(yytext);*/ return T_string; }

"--"([^\n])*\n			{ linecount++; }

<*>"(*"			{ BEGIN(COMMENT); commentcount++; }
<COMMENT>"*)"		{ if(--commentcount==0) BEGIN(INITIAL);  }
<COMMENT>"*"		{ /* nothing */ }
<COMMENT>[^*\n]+    	{ /* nothing */ }

"="	{ return T_assign; }
"+"	{ return '+'; }
"-"	{ return '-'; }
"*"	{ return '*'; }
"/"	{ return '/'; }
"%"     { return T_mod; }
"!"     { return T_excl; }
"&"	{ return T_and; }
"|"	{ return T_or; }
"=="	{ return T_eq; }
"!="	{ return T_ne; }
"<"	{ return T_lt; }
"<="	{ return T_le; }
">"	{ return T_gt; }
">="	{ return T_ge; }

"("	{ return T_oppar; }
")"	{ return T_clpar; }
"["	{ return T_opj; }
"]"	{ return T_clj; }
"{"	{ return T_begin; }
"}"	{ return T_end; }
":"	{ return T_dd; }
","	{ return T_comma; }
";"	{ return T_semic; }

<INITIAL><<EOF>>	{ return T_eof; }

%{
/********************** 
* ERRORS AND WARNINGS *
**********************/
%}

<COMMENT><<EOF>>	{ fprintf(stderr, "Lexical error at line %d: "
			"Comments should close\n",linecount); exit(1);}

<INITIAL>"*)"	{ fprintf(stderr, "Lexical error at line %d: "
		"Comments close without opening\n",linecount); }

%{
/*
{digit}+	{ if(atoi(yytext) <= 32768) return T_constnum;
//		else if(atoi(yytext) == 32768) fprintf(stderr, "Warning at line %d: "
//		"Number is at the limit\n",linecount);
//		else fprintf(stderr, "Error at line %d: "
//		"Number is out of limit\n",linecount); }
*/
%}

\"({simplechar}|{white}|\\)*\n	{ fprintf(stderr, "Lexical error at line %d: "
				"String starts without ending at the same line\n",linecount); }

\'({simplechar}|{specialchar})*	{ fprintf(stderr, "Lexical error at line %d: "
				"Character should be in ' '\n",linecount); }

.	{ fprintf(stderr, "Lexical error at line %d: "
		"Illigal character %c "
		"with ASCII code %d\n",
		linecount, yytext[0], yytext[0]); }



%%

int installID()
{
	static int id_count = 0;
	
	return (id_count++); 
}

int installNum()
{
	return (atoi(yytext));
}



