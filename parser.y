%{
#include <stdio.h>
#include <stdlib.h>

void yyerror (const char msg[]);
int yylex(void);

int linecount=1;
%}

%token T_eof 0
%token T_byte T_int T_proc T_reference T_return T_while 
%token T_if T_else
%token T_true T_false
%token T_id T_constnum T_constchar T_string
%token T_assign T_mod T_excl T_and T_or T_eq T_ne T_lt T_le T_gt T_ge
%token T_oppar T_clpar T_opj T_clj T_begin T_end
%token T_dd T_comma T_semic
%token T_newline

%left T_or
%left T_and
%nonassoc T_eq T_ne T_gt T_lt T_ge T_le
%left '+' '-'
%left '*' '/' T_mod
%right UPLUS UMINUS T_excl

%union {
	int		i;
	char		c;
	char*	s;
}
	
%error-verbose

%%

program		:	func_def
			;
func_def		:	T_id T_oppar T_clpar T_dd r_type local_def0 compound_stmt
			|	T_id T_oppar fpar_list T_clpar T_dd r_type local_def0 compound_stmt
			;
local_def0	:	/*EMPTY*/
			|	local_def0 local_def
			;
fpar_list		:	fpar_def fpar_list0
			;
fpar_list0	:	/*EMPTY*/
			|	fpar_list0 T_comma fpar_def
			;
fpar_def		:	T_id T_dd type
			|	T_id T_dd T_reference type
			;
data_type		:	T_int
			|	T_byte
			;
type			:	data_type
			|	data_type T_opj T_clj
			;
r_type		:	data_type
			|	T_proc
			;
local_def		:	func_def
			|	var_def
			;
var_def		:	T_id T_dd data_type T_semic
			|	T_id T_dd data_type T_opj T_constnum T_clj T_semic
			;
stmt			:	T_semic
			|	l_value T_assign expr T_semic
			|	compound_stmt
			|	func_call T_semic
			|	T_if T_oppar cond T_clpar stmt
			|	T_if T_oppar cond T_clpar stmt T_else stmt
			|	T_while T_oppar cond T_clpar stmt
			|	T_return T_semic
			|	T_return expr T_semic
			;
compound_stmt	:	T_begin compound_stmt0 T_end
			;
compound_stmt0	:	/*EMPTY*/
			|	compound_stmt0 stmt
			;
func_call		:	T_id T_oppar T_clpar
			|	T_id T_oppar expr_list T_clpar
			;
expr_list		:	expr expr_list0
			;
expr_list0	:	/*EMPTY*/
			|	expr_list0 T_comma expr
			;
expr			:	T_constnum
			|	T_constchar
			|	l_value
			|	T_oppar expr T_clpar
			|	func_call
			|	'+' expr %prec UPLUS
			|	'-' expr %prec UMINUS
			|	expr '+' expr
			|	expr '-' expr
			|	expr '*' expr
			|	expr '/' expr
			|	expr T_mod expr
			;
l_value		:	T_id
			|	T_id T_opj expr T_clj
			|	T_string
			;
cond			:	T_true 
			|	T_false
			|	T_oppar cond T_clpar
			|	T_excl cond
			|	expr T_eq expr
			|	expr T_ne expr
			|	expr T_gt expr
			|	expr T_lt expr
			|	expr T_ge expr
			|	expr T_le expr
			|	cond T_and cond
			|	cond T_or cond
			;
	
%%

void yyerror (const char * msg)
{
  fprintf(stderr, "syntax error in line %d: %s\n", linecount, msg);
  exit(1);
}

int main ()
{
  return yyparse();
}


















