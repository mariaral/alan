%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol.h" /*includes functions of symbol table*/ 
#include "error.h"
#include "general.h"

SymbolEntry *fun_decl, *fun_call;
SymbolEntry *lval;
SymbolEntry *currentArg;
SymbolEntry *currrentFunction;
bool many_arg;
bool ret_exists, ret_at_end;
void yyerror (const char msg[]);
bool int_or_byte(Type a,Type b);
bool equalArrays(Type a,Type b);
int yylex(void);
int linecount=1;
%}

%union {
	int	i;
	char	c;
	char	s[32];
	Type	t;
	bool	b;
}

/*Bison declarations */
%token T_eof 0
%token T_byte T_int T_proc T_reference T_return T_while 
%token T_if T_else
%token T_true T_false
%token<s> T_id T_string
%token<i> T_constnum 
%token T_constchar
%token T_assign T_mod T_excl T_and T_or T_eq T_ne T_lt T_le T_gt T_ge
%token T_oppar T_clpar T_opj T_clj T_begin T_end
%token T_dd T_comma T_semic
%token T_newline

/* operator precedence */
%left T_or
%left T_and
%nonassoc T_eq T_ne T_gt T_lt T_ge T_le
%left '+' '-'
%left '*' '/' T_mod
%right UPLUS UMINUS T_excl

%type<t> type data_type r_type expr func_call
%type<t> l_value

%error-verbose
%expect 1

%%

program		:	{ openScope(); init_ready_functions(); }	func_def	{ closeScope() }
		;
		
func_def	:	T_id					{ fun_decl = newFunction($1); openScope() }
			T_oppar fpar_list T_clpar T_dd r_type	{ endFunctionHeader(fun_decl,$7) }
			local_def0	{ ret_at_end=false; ret_exists=false } 
			compound_stmt	{ currrentFunction = currentScope->parent->entries;
					  if((!ret_exists)&&(!equalType(currrentFunction->u.eFunction.resultType,typeVoid)))
					  	error("Non proc functions must return value");
					  else if((ret_exists)&&(!ret_at_end))
					  	warning("Function doesn't return at end");
					  closeScope() }
		;
			
local_def0	:	/*EMPTY*/
		|	local_def0 local_def
		;
			
fpar_list	:	/*EMPTY*/	
		|	fpar_def fpar_list0
		;
			
fpar_list0	:	/*EMPTY*/	
		|	fpar_list0 T_comma fpar_def
		|	fpar_list0 fpar_def	{ error("parameters must be separated by ,") }
		;
			
fpar_def	:	T_id T_dd type	{ if($3->kind != TYPE_IARRAY)
					  	newParameter($1, $3, PASS_BY_VALUE, fun_decl);
					  else error("Arrays must always pass by reference") }
		|	T_id T_dd T_reference type	{ newParameter($1, $4, PASS_BY_REFERENCE, fun_decl) }
		;
			
data_type	:	T_int	{ $$ = typeInteger }
		|	T_byte	{ $$ = typeChar    }
		;
			
type		:	data_type		{ $$ = $1 }
		|	data_type T_opj T_clj	{ $$ = typeIArray($1) }
		;
			
r_type		:	data_type	{ $$ = $1 }
		|	T_proc		{ $$ = typeVoid }
		;
			
local_def	:	func_def
		|	var_def
		;
			
var_def		:	T_id T_dd data_type T_semic	{ newVariable($1, $3) }
		|	T_id T_dd data_type T_opj T_constnum T_clj T_semic	{ newVariable($1,typeArray($5,$3)) }
		;
		
stmt		:	T_semic	{ ret_at_end = false }
		|	l_value T_assign expr T_semic	{ if($1->kind==TYPE_ARRAY) error("Left value cannot be type Array");
							  else if($1!=$3) error("Expression must be same type with left value");
							  ret_at_end = false }
		|	compound_stmt		{ ret_at_end = false }
		|	func_call T_semic	{ if($1!=typeVoid) error("Function must have type proc");
						  ret_at_end = false }
		|	T_if T_oppar cond T_clpar stmt		{ ret_at_end = false }
		|	T_if T_oppar cond T_clpar stmt T_else stmt	{ ret_at_end = false }
		|	T_while T_oppar cond T_clpar stmt	{ ret_at_end = false }
		|	T_return T_semic	{ currrentFunction = currentScope->parent->entries;
						  if(currrentFunction->u.eFunction.resultType!=typeVoid)
						  	error("Function returns no value");
						  ret_at_end = true;
						  ret_exists = true
						 }
		|	T_return expr T_semic	{ currrentFunction = currentScope->parent->entries;
						  if(currrentFunction->u.eFunction.resultType!=$2)
						  	error("Function must return same type of value as declared");
						ret_at_end = true;
						ret_exists = true
						}
		;
		
compound_stmt	:	T_begin compound_stmt0 T_end
		;
		
compound_stmt0	:	/*EMPTY*/
		|	stmt compound_stmt0 
		;
		
func_call	:	T_id T_oppar	{ if ((fun_call=lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL) 
					  	fatal("Identifier cannot be found");
					  currentArg = fun_call->u.eFunction.firstArgument; }
			expr_list T_clpar	{ $$ = fun_call->u.eFunction.resultType }
		;
		
expr_list	:	/*EMPTY*/	
		|	expr_list0
		;
		
expr_list0	:	expr	{ if(many_arg)
				  	many_arg=false;
				  else { 
					if(currentArg!=NULL) {
  			  	  		if((!equalType($1,currentArg->u.eParameter.type))&&(!equalArrays($1,currentArg->u.eParameter.type)))
				  			error("Wrong type of parameter1");
				  		if(currentArg->u.eParameter.next != NULL)
				  			error("Too few arguments at call2");
				  	} 
				  	else error("Too many arguments at call");
				  } 
				}
		|	expr	{ if(!many_arg) {
				  	if(currentArg!=NULL) {
				  		if((!equalType($1,currentArg->u.eParameter.type))&&(!equalArrays($1,currentArg->u.eParameter.type)))
				  			error("Wrong type of parameter");
				  		currentArg = currentArg->u.eParameter.next;
				  	} 
				  	else {  error("Too many arguments at call1");
				    	    many_arg = true; 
				    	}
				   } 
				 }
		T_comma expr_list0	
		;
		
expr		:	T_constnum	{ $$ = typeInteger }
		|	T_constchar	{ $$ = typeChar }
		|	l_value		{ $$ = $1 }
		|	T_oppar expr T_clpar	{ $$ = $2 }
		|	func_call	{ if($1!=typeVoid) $$=$1;
					  else error("Cannot call proc") }
			
		|	'+' expr %prec UPLUS	{ if($2!=typeInteger)
							error("Opperand must be type int");
						  else  $$ = $2 }
						  
		|	'-' expr %prec UMINUS	{ if($2!=typeInteger)
							error("Opperand must be type int");
						  else  $$ = $2 }
						  
		|	expr '+' expr		{ if(!int_or_byte($1,$3))
						  	error("Opperands must be type int or byte");
						  else {
						  	if($1 != $3)
						  		error("Operands must be same type");
						  	else $$ = $1;
						  } }
		|	expr '-' expr		{ if(!int_or_byte($1,$3))
						  	error("Opperands must be type int or byte");
						  else {
						  	if($1 != $3)
						  		error("Operands must be same type");
						  	else $$ = $1;
						  } }						  
		|	expr '*' expr		{ if(!int_or_byte($1,$3))
						  	error("Opperands must be type int or byte");
						  else {
						  	if($1 != $3)
						  		error("Operands must be same type");
						  	else $$ = $1;
						  } }	  						  
		|	expr '/' expr		{ if(!int_or_byte($1,$3))
						  	error("Opperands must be type int or byte");
						  else {
						  	if($1 != $3)
						  		error("Operands must be same type");
						  	else $$ = $1;
						  } }						  
		|	expr T_mod expr		{ if(!int_or_byte($1,$3))
						  	error("Opperands must be type int or byte");
						  else {
						  	if($1 != $3)
						  		error("Operands must be same type");
						  	else $$ = $1;
						  } }						  
		;
		
l_value		:	T_id			{ if((lval = lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL)
						  	fatal("Identifier cannot be found");
				 		  if(lval->entryType == ENTRY_VARIABLE)
				 		  	$$ = lval->u.eVariable.type;
				  		  else if(lval->entryType == ENTRY_PARAMETER)
				  		  	$$ = lval->u.eParameter.type;
				  		  else error("Identifiers don't match") }
		|	T_id T_opj expr T_clj	{ if($3 != typeInteger) 
						  	error("Array index must be integer");
						  if((lval = lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL)
						  	fatal("Identifier cannot be found");
				 		  if(lval->entryType == ENTRY_VARIABLE)
						  	$$ = lval->u.eVariable.type->refType;
				   		  else if(lval->entryType == ENTRY_PARAMETER)
				   		  	$$ = lval->u.eParameter.type->refType;
			  			  else error("Identifiers don't match") }
		|	T_string		{ $$ = typeArray(strlen($1)+1,typeChar); } 
		;
		
cond		:	T_true
		|	T_false
		|	T_oppar cond T_clpar
		|	T_excl cond
		|	expr T_eq expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	expr T_ne expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	expr T_gt expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	expr T_lt expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	expr T_ge expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	expr T_le expr	{ if(!int_or_byte($1,$3)) error("Opperands must be int or byte");
					  else if($1!=$3) error("Cannot compaire different types") }
		|	cond T_and cond
		|	cond T_or cond
		;
	
%%

bool int_or_byte(Type a,Type b)
{
return (((a==typeInteger)||(a==typeChar))&&((b==typeInteger)||(b==typeChar)));
}

bool equalArrays(Type a,Type b)
{
	if(a->kind!=TYPE_ARRAY) return false;
	else if (b->kind==TYPE_IARRAY) return true;
	else return false;
}

void yyerror (const char * msg)
{
  fprintf(stderr, "Syntax error in line %d: %s\n", linecount, msg);
  exit(1);
}

int main ()
{
	initSymbolTable(257);
		
 	return yyparse();
}
