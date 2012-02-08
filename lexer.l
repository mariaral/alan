%{
/*
****************************************************
             LEXICAL ANALYSER-FLEX FILE
****************************************************
*/

#include<stdio.h>

#define T_eof	0
#define T_byte	1000
#define T_else	1001
#define T_false	1002
#define T_if	1003
#define T_int	1004
#define T_proc	1005
#define T_reference	1006
#define T_return	1007
#define T_while		1008
#define T_true		1009
#define T_constnum	1010
#define T_id		1011
#define T_constchar	1012
#define T_string	1013
#define T_assign	1014
#define T_plus	1015
#define T_minus	1016
#define T_mul	1017
#define T_div	1018
#define T_mod	1019
#define T_excl	1020
#define T_and	1021
#define T_or	1022
#define T_eq	1023
#define T_ne	1024
#define T_lt	1025
#define T_le	1026
#define T_gt	1027
#define T_ge	1028
#define T_oppar	1029
#define T_clpar	1030
#define T_opj	1031
#define T_clj	1032
#define T_begin	1033
#define T_end	1034
#define T_dd	1035
#define T_comma	1036
#define T_semic	1037

int linecount=1;
int commentcount=0;

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
<*>\n		{ linecount++; }

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

{letter}({letter}|{digit})*		{ return T_id; }
\'({simplechar}|{specialchar})\'	{ return T_constchar; }
\"({simplechar}|{white}|\\)*\"		{ return T_string; }

"--"([^\n])*\n			{ linecount++; }

<*>"(*"			{ BEGIN(COMMENT); commentcount++; }
<COMMENT>"*)"		{ if(--commentcount==0) BEGIN(INITIAL);  }
<COMMENT>"*"		{ /* nothing */ }
<COMMENT>[^*\n]+    	{ /* nothing */ }

"="	{ return T_assign; }
"+"	{ return T_plus; }
"-"	{ return T_minus; }
"*"	{ return T_mul; }
"/"	{ return T_div; }
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

{digit}+	{ if(atoi(yytext) <= 32768) return T_constnum;
		else if(atoi(yytext) == 32768) fprintf(stderr, "Warning at line %d: "
		"Number is at the limit\n",linecount);
		else fprintf(stderr, "Error at line %d: "
		"Number is out of limit\n",linecount); }

\"({simplechar}|{white}|\\)*\n	{ fprintf(stderr, "Lexical error at line %d: "
				"String starts without ending at the same line\n",linecount); }

\'({simplechar}|{specialchar})*	{ fprintf(stderr, "Lexical error at line %d: "
				"Character should be in ' '\n",linecount); }

.	{ fprintf(stderr, "Lexical error at line %d: "
		"Illigal character %c "
		"with ASCII code %d\n",
		linecount, yytext[0], yytext[0]); }



%%

int main ()
{
   int token;
   do {
      token = yylex();
      printf("token=%d, lexeme=\"%s\"\n",
             token, yytext);
   } while (token != T_eof);
   return 0;
}

