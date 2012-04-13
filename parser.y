%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include "error.h"
#include "symbol.h"
#include "quad.h"

SymbolEntry *fun_decl, *fun_call;
SymbolEntry *lval;
SymbolEntry *currentArg;
SymbolEntry *currrentFunction;
operand op0,op1,op2;
bool many_arg;
bool ret_exists, ret_at_end;
void yyerror (const char msg[]);
bool int_or_byte(Type a,Type b);
bool equalArrays(Type a,Type b);
int yylex(void);
int linecount=1;
%}

%union {
        int     i;
        char    c;
        Type type;
        char s[32];
        struct {
                Place place;
                Type type;
        } var;
        labelList *Next;
        struct {
                labelList *True;
                labelList *False;
        } b;
        char Name[256];
};



/*Bison declarations */
%token T_eof 0
%token T_byte T_int T_proc T_reference T_return T_while 
%token T_if T_else
%token T_true T_false
%token<Name> T_id T_string
%token<i> T_constnum 
%token<c> T_constchar
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

%type<type> type data_type func_call r_type
%type<var> l_value expr 
%type<b> cond
%type<Next> stmt

%error-verbose
%expect 1

%%

program		:	{ openScope(); init_ready_functions(); }	func_def	{ closeScope(); }
		;
		
func_def	:	T_id					{ fun_decl = newFunction($1); openScope(); }
			T_oppar fpar_list T_clpar T_dd r_type	{ endFunctionHeader(fun_decl,$7); }
			local_def0	{ ret_at_end=false; ret_exists=false; } 
			compound_stmt	{ currrentFunction = currentScope->parent->entries;
					  if((!ret_exists)&&(!equalType(currrentFunction->u.eFunction.resultType,typeVoid)))
					  	error("Non proc functions must return value");
					  else if((ret_exists)&&(!ret_at_end))
					  	warning("Function doesn't return at end");
					  closeScope(); }
		;
			
local_def0	:	/*EMPTY*/
		|	local_def0 local_def
		;
			
fpar_list	:	/*EMPTY*/	
		|	fpar_def fpar_list0
		;
			
fpar_list0	:	/*EMPTY*/	
		|	fpar_list0 T_comma fpar_def
		|	fpar_list0 fpar_def	{ error("parameters must be separated by ,"); }
		;
			
fpar_def	:	T_id T_dd type	{ if($3->kind != TYPE_IARRAY)
					  	newParameter($1, $3, PASS_BY_VALUE, fun_decl);
					  else {
					  	error("Arrays must always pass by reference");
					  	newParameter($1, $3, PASS_BY_REFERENCE, fun_decl);	
					  	} }
		|	T_id T_dd T_reference type	{ newParameter($1, $4, PASS_BY_REFERENCE, fun_decl); }
		;
			
data_type	:	T_int	{ $$ = typeInteger; }
		|	T_byte	{ $$ = typeChar;    }
		;
			
type		:	data_type		{ $$ = $1; }
		|	data_type T_opj T_clj	{ $$ = typeIArray($1); }
		;
			
r_type		:	data_type	{ $$ = $1; }
		|	T_proc		{ $$ = typeVoid; }
		;
			
local_def	:	func_def
		|	var_def
		;
			
var_def		:	T_id T_dd data_type T_semic	{ newVariable($1, $3); }
		|	T_id T_dd data_type T_opj T_constnum T_clj T_semic	{ newVariable($1,typeArray($5,$3)); }
		;
		
stmt		:	T_semic	{ ret_at_end = false; }
		|	l_value T_assign expr T_semic	{ if(($1.type)->kind==TYPE_ARRAY) error("Left value cannot be type Array");
							  else if($1.type!=$3.type) error("Expression must be same type with left value");
							  ret_at_end = false; }
		|	compound_stmt		{ ret_at_end = false; }
		|	func_call T_semic	{ if(!equalType($1,typeVoid)) error("Function must have type proc");
						  ret_at_end = false; }
		|	T_if T_oppar cond T_clpar stmt		{ ret_at_end = false; }
		|	T_if T_oppar cond T_clpar stmt T_else stmt	{ ret_at_end = false; }
		|	T_while T_oppar cond T_clpar stmt	{ ret_at_end = false; }
		|	T_return T_semic	{ currrentFunction = currentScope->parent->entries;
						  if(currrentFunction->u.eFunction.resultType!=typeVoid)
						  	error("Function returns no value");
						  ret_at_end = true;
						  ret_exists = true;
						 }
		|	T_return expr T_semic	{ currrentFunction = currentScope->parent->entries;
						  if(currrentFunction->u.eFunction.resultType!=$2.type)
						  	error("Function must return same type of value as declared");
						ret_at_end = true;
						ret_exists = true;
						}
		;
		
compound_stmt	:	T_begin compound_stmt0 T_end
		;
		
compound_stmt0	:	/*EMPTY*/
		|	stmt compound_stmt0 
		;
		
func_call	:	T_id T_oppar	{ if((fun_call=lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL) 
					  	fatal("Identifier cannot be found");
					  if(fun_call->entryType!=ENTRY_FUNCTION)
					  	fatal("Identifier is not a function"); 
					  currentArg = fun_call->u.eFunction.firstArgument; }
			expr_list T_clpar	{ $$ = fun_call->u.eFunction.resultType; }
		;
		
expr_list	:	/*EMPTY*/	
		|	expr_list0
		;
		
expr_list0	:	expr	{ if(many_arg)
				  	many_arg=false;
				  else { 
					if(currentArg!=NULL) {
  			  	  		if((!equalType($1.type,currentArg->u.eParameter.type))&&(!equalArrays($1.type,currentArg->u.eParameter.type)))
				  			error("Wrong type of parameter");
				  		if(currentArg->u.eParameter.next != NULL)
				  			error("Too few arguments at call");
				  	} 
				  	else error("Too many arguments at call");
				  } 
				}
		|	expr	{ if(!many_arg) {
				  	if(currentArg!=NULL) {
				  		if((!equalType($1.type,currentArg->u.eParameter.type))&&(!equalArrays($1.type,currentArg->u.eParameter.type)))
				  			error("Wrong type of parameter");
				  		currentArg = currentArg->u.eParameter.next;
				  	} 
				  	else {  error("Too many arguments at call");
				    	    many_arg = true; 
				    	}
				   } 
				 }
		T_comma expr_list0	
		;
		
expr		:	T_constnum	{ $$.type = typeInteger;
                                          $$.place.placeType = CONSTNUM;
                                          $$.place.u.constnum = $1; }

		|	T_constchar	{ $$.type = typeChar;
                                          $$.place.placeType = CONSTCHAR;
                                          $$.place.u.constchar = $1; }

		|	l_value		{ $$.type = $1.type;
                                          $$.place = $1.place; }

		|	T_oppar expr T_clpar	{ $$.type = $2.type;
                                                  $$.place = $2.place;}

		|	func_call	{ if($1!=typeVoid) { 
                                                $$.type = $1;
                                                
                                          }
					  else error("Cannot call proc"); }
			
		|	'+' expr %prec UPLUS	{ if($2.type!=typeInteger)
							error("Opperand must be type int");
						  else { $$.type = $2.type;
                                                         $$.place = $2.place; } }
						  
		|	'-' expr %prec UMINUS	{ if($2.type!=typeInteger)
							error("Opperand must be type int");
						  else {
                                                        $$.type = $2.type;
                                                        op0.opType = OP_PLACE;
                                                        op0.u.place = $2.place;
                                                        op1.opType = OP_NOTHING;
                                                        op2.opType = OP_PLACE;
                                                        op2.u.place.placeType = ENTRY;
                                                        op2.u.place.u.entry = newTemporary($2.type);
                                                        genQuad(MINUS,op0,op1,op2);
                                                        $$.place = op2.u.place; } }
						  
		|	expr '+' expr		{ if(!int_or_byte($1.type,$3.type))
						  	error("Operands must be type int or byte");
						  else {
						  	if($1.type != $3.type)
						  		error("Operands must be same type");
						  	else {
                                                                $$.type = $1.type;
                                                                op0.opType = OP_PLACE;
                                                                op0.u.place = $1.place;
                                                                op1.opType = OP_PLACE;
                                                                op1.u.place = $3.place;
                                                                op2.opType = OP_PLACE;
                                                                op2.u.place.placeType = ENTRY;
                                                                op2.u.place.u.entry = newTemporary($3.type);
                                                                genQuad(PLUS,op0,op1,op2);
                                                                $$.place = op2.u.place;
                                                        }

						  } }
		|	expr '-' expr		{ if(!int_or_byte($1.type,$3.type))
						  	error("Operands must be type int or byte");
						  else {
						  	if($1.type != $3.type)
						  		error("Operands must be same type");
						  	else {
                                                                $$.type = $1.type;
                                                                op0.opType = OP_PLACE;
                                                                op0.u.place = $1.place;
                                                                op1.opType = OP_PLACE;
                                                                op1.u.place = $3.place;
                                                                op2.opType = OP_PLACE;
                                                                op2.u.place.placeType = ENTRY;
                                                                op2.u.place.u.entry = newTemporary($3.type);
                                                                genQuad(MINUS,op0,op1,op2);
                                                                $$.place = op2.u.place;
                                                        }


						  } }						  
		|	expr '*' expr		{ if(!int_or_byte($1.type,$3.type))
						  	error("Operands must be type int or byte");
						  else {
						  	if($1.type != $3.type)
						  		error("Operands must be same type");
						  	else {
                                                                $$.type = $1.type;
                                                                op0.opType = OP_PLACE;
                                                                op0.u.place = $1.place;
                                                                op1.opType = OP_PLACE;
                                                                op1.u.place = $3.place;
                                                                op2.opType = OP_PLACE;
                                                                op2.u.place.placeType = ENTRY;
                                                                op2.u.place.u.entry = newTemporary($3.type);
                                                                genQuad(MULT,op0,op1,op2);
                                                                $$.place = op2.u.place;
                                                        }

						  } }	  						  
		|	expr '/' expr		{ if(!int_or_byte($1.type,$3.type))
						  	error("Operands must be type int or byte");
						  else {
						  	if($1.type != $3.type)
						  		error("Operands must be same type");
						  	else {
                                                                $$.type = $1.type;
                                                                op0.opType = OP_PLACE;
                                                                op0.u.place = $1.place;
                                                                op1.opType = OP_PLACE;
                                                                op1.u.place = $3.place;
                                                                op2.opType = OP_PLACE;
                                                                op2.u.place.placeType = ENTRY;
                                                                op2.u.place.u.entry = newTemporary($3.type);
                                                                genQuad(DIVI,op0,op1,op2);
                                                                $$.place = op2.u.place;
                                                        }

						  } }						  
		|	expr T_mod expr		{ if(!int_or_byte($1.type,$3.type))
						  	error("Operands must be type int or byte");
						  else {
						  	if($1.type != $3.type)
						  		error("Operands must be same type");
						  	else {
                                                                $$.type = $1.type;
                                                                op0.opType = OP_PLACE;
                                                                op0.u.place = $1.place;
                                                                op1.opType = OP_PLACE;
                                                                op1.u.place = $3.place;
                                                                op2.opType = OP_PLACE;
                                                                op2.u.place.placeType = ENTRY;
                                                                op2.u.place.u.entry = newTemporary($3.type);
                                                                genQuad(MOD,op0,op1,op2);
                                                                $$.place = op2.u.place;
                                                        }

						  } }						  
		;
		
l_value		:	T_id			{ if((lval = lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL)
						  	fatal("Identifier cannot be found");
				 		  if(lval->entryType == ENTRY_VARIABLE) {
				 		  	$$.type = lval->u.eVariable.type;
                                                        $$.place.placeType = ENTRY;
                                                        $$.place.u.entry = lval;
                                                  }
				  		  else {
                                                  if(lval->entryType == ENTRY_PARAMETER) {
				  		  	$$.type = lval->u.eParameter.type;
                                                        $$.place.placeType = ENTRY; 
                                                        $$.place.u.entry = lval;
                                                  }
				  		  else error("Identifiers don't match"); } }
		|	T_id T_opj expr T_clj	{ if($3.type != typeInteger) 
						  	error("Array index must be integer");
						  if((lval = lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL)
						  	fatal("Identifier cannot be found");
				 		  if(lval->entryType == ENTRY_VARIABLE) {
						  	$$.type = lval->u.eVariable.type->refType;
                                                        genQuad(ARRAY,op0,op1,op2);
                                                        $$.place.placeType = ENTRY; 
                                                        $$.place.u.entry = lval;
                                                  }

				   		  else {
                                                  if(lval->entryType == ENTRY_PARAMETER) {
				   		  	$$.type = lval->u.eParameter.type->refType;
                                                        $$.place.placeType = ENTRY; 
                                                        $$.place.u.entry = lval;
                                                  }
                                                  else error("Identifiers don't match"); } }
		|	T_string		{ $$.type = typeArray(strlen($1)+1,typeChar);
                                                  $$.place.placeType = STRING;
                                                  strcpy($1,$$.place.u.string); } 
		;
		
cond		:	T_true
		|	T_false
		|	T_oppar cond T_clpar
		|	T_excl cond
		|	expr T_eq expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
		|	expr T_ne expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
		|	expr T_gt expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
		|	expr T_lt expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
		|	expr T_ge expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
		|	expr T_le expr	{ if(!int_or_byte($1.type,$3.type)) error("Opperands must be int or byte");
					  else if($1.type!=$3.type) error("Cannot compaire different types"); }
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
