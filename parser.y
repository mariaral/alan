%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include "error.h"
#include "symbol.h"
#include "quad.h"
#include "libalan.h"
#include "typecheck.h"


SymbolEntry *fun_decl, *fun_call;
SymbolEntry *lval;
SymbolEntry *currentArg, *arg;
SymbolEntry *currrentFunction;
Place temp;
labelList *L;
Type retType;
bool many_arg, typeError;
bool global_typeError;
bool ret_exists, ret_at_end;
void yyerror (const char msg[]);
int yylex(void);
int linecount=1;
int const_counter=0;
char buff[10];
%}

%union {
        int     i;
        char    c;
        oper op;
        Type type;
        varstr var;
        labelList *Next;
        boolean b;
        char Name[256];
        struct {
           labelList *L1;
           labelList *L2;
        } l;

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
%left T_plus T_minus
%left T_mult T_div T_mod
%right UPLUS UMINUS T_excl

%type<type> type data_type r_type
%type<var> l_value lval_id expr func_call
%type<b> cond
%type<Next> stmt compound_stmt compound_stmt0

%error-verbose
%expect 1

%%

program		:   { global_typeError = false;
                  openScope();
                  init_ready_functions(); }
                  func_def	{ closeScope(); }
            ;

func_def	:	T_id    { fun_decl = lookupEntry($1,LOOKUP_CURRENT_SCOPE,false);
                          if(fun_decl!=NULL) {
                            error("Duplicate declaration of function");
                            destroyLocalEntry(fun_decl);
                          } 
                          fun_decl = newFunction($1);
                          openScope(); }

                T_oppar fpar_list T_clpar T_dd r_type	
                { endFunctionHeader(fun_decl,$7); }

                local_def0  { ret_at_end=false; 
                              ret_exists=false;  
                              quadLast = NULL;
                              genQuad(UNIT,op(OP_NAME,$1),op(OP_NOTHING),op(OP_NOTHING)); }

                compound_stmt   { currrentFunction = currentScope->parent->entries;
                                  if((!ret_exists)&&(!equalType(currrentFunction->u.eFunction.resultType,typeVoid))) 
                                    error("Non proc functions must return value");


                                  else
                                  if((ret_exists)&&(!ret_at_end))
                                    warning("Function doesn't return at end");
                                  backpatch($11, nextQuad());
                                  genQuad(ENDU,op(OP_NAME,$1),op(OP_NOTHING),op(OP_NOTHING));
                                  if(!global_typeError)
                                    printQuads();
                                  closeScope(); }
            ;

local_def0  :	/*EMPTY*/
            |	local_def0 local_def
            ;

fpar_list	:	/*EMPTY*/
            |	fpar_def fpar_list0
            ;

fpar_list0  :	/*EMPTY*/
            |	fpar_list0 T_comma fpar_def
            |	fpar_list0 fpar_def	{ error("parameters must be separated by ,");
                                          global_typeError = true; }
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
    
type        :	data_type		        { $$ = $1; }
            |	data_type T_opj T_clj	{ $$ = typeIArray($1); }
            ;

r_type	    :	data_type	{ $$ = $1; }
            |	T_proc		{ $$ = typeVoid; }
            ;

local_def   :	func_def
            |	var_def
            ;

var_def	    :	T_id T_dd data_type T_semic	{ newVariable($1, $3); }
            |   T_id T_dd data_type T_opj T_constnum T_clj T_semic	{ newVariable($1,typeArray($5,$3)); }
            ;

stmt        :	T_semic	{ ret_at_end = false; $$ = emptyList(); }
            |	l_value T_assign expr T_semic	{ if(($1.type)->kind==TYPE_ARRAY) {
                                                    error("Left value cannot be type Array");
                                                    ret_at_end = false;
                                                    $$ = emptyList();
                                                    break;
                                                  }
                                                  if(unknownType($1.type,$3.type)) {
                                                    ret_at_end = false;
                                                    $$ = emptyList();
                                                    break;
                                                  }
                                                  if($1.type!=$3.type) {
                                                    error("Expression must be same type with left value");
                                                    ret_at_end = false;                                                                              $$ = emptyList();
                                                    break;
                                                  }
                                                  genQuad(ASSIGN,op(OP_PLACE,$3.place),op(OP_NOTHING),op(OP_PLACE,$1.place));
                                                  ret_at_end = false;
                                                  $$ = emptyList(); }

            |	compound_stmt		{ ret_at_end = false; $$ = $1; }

            |	func_call T_semic	{ if((!equalType($1.type,typeVoid))&&(!equalType($1.type,typeUnknown))) 
                                            warning("Function result is not used");
                                          $$ = emptyList();
                                          ret_at_end = false; }

            |	T_if T_oppar cond T_clpar { backpatch($3.True,nextQuad());
                                            $<l>$.L1 = $3.False;
                                            $<l>$.L2 = emptyList(); } 

                stmt else_stmt { $$ = merge(merge($6,$<l>7.L1),$<l>7.L2);
                                 ret_at_end = false; }

            |	T_while { $<i>$ = nextQuad(); } 

                T_oppar cond T_clpar { backpatch($4.True,nextQuad()); }

                stmt	{ backpatch($7,$<i>2);
                          genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_LABEL,$<i>2));
                          $$ = $4.False;
                          ret_at_end = false; }

            |	T_return T_semic	{ currrentFunction = currentScope->parent->entries;
                                      if(currrentFunction->u.eFunction.resultType!=typeVoid) 
                                        error("Function returns no value");
                                      genQuad(RET,op(OP_NOTHING),op(OP_NOTHING),op(OP_NOTHING));
                                      ret_at_end = true;
                                      ret_exists = true;
                                      $$ = emptyList(); }

            |	T_return expr T_semic	{ currrentFunction = currentScope->parent->entries;
                                          if(currrentFunction->u.eFunction.resultType!=$2.type) 
                                          
                                            error("Function must return same type of value as declared");
                                          genQuad(RET,op(OP_NOTHING),op(OP_NOTHING),op(OP_NOTHING));
                                          ret_at_end = true;
                                          ret_exists = true;
                                          $$ = emptyList(); }
            ;

else_stmt   :       /*EMPTY*/ { $<l>$ = $<l>-1; }
            |       T_else    { genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
                                $<Next>$ = makeList(currentQuad());
                                backpatch($<b>-3.False,nextQuad()); }
                    stmt      { $<l>$.L2 = $3;
                                $<l>$.L1 = $<Next>2; }
            ;

compound_stmt	:	T_begin { L = emptyList(); } 
                    compound_stmt0  T_end { $$ = $3; }
                ;

compound_stmt0	:	/*EMPTY*/ { $$ = L; }
                |	{ backpatch(L,nextQuad()); } 
                    stmt { L = $2; }
                    compound_stmt0 { $$ = $4; }
                ;

func_call     :	T_id T_oppar	{ typeError = false;
                                  if((fun_call=lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL) {
                                  /*  error("Identifier cannot be found"); */
                                    typeError = true;
                                    fun_decl = newFunction($1);
                                    endFunctionHeader(fun_decl,typeUnknown);
                                    break;
                                  }
                                  if(fun_call->entryType!=ENTRY_FUNCTION) {
                                    error("Identifier %s is not a function",$1);
                                    typeError = true;
                                    break;
                                  }
                                  if(fun_call->u.eFunction.resultType==typeUnknown) {
                                    typeError = true;
                                    break;
                                  }
                                  currentArg = fun_call->u.eFunction.firstArgument; }
                 expr_list T_clpar  { if(typeError) {
                                        $$.type = typeUnknown;
                                        typeError = false;
                                        break;
                                      }
                                      if((retType=fun_call->u.eFunction.resultType)!=typeVoid) {
                                        temp=newTemp(retType);
                                        genQuad(PAR,op(OP_RESULT),op(OP_PLACE,temp),op(OP_NOTHING));
                                        $$.place = temp;
                                       }
                                       $$.type = retType;
                                       genQuad(CALL,op(OP_NOTHING),op(OP_NOTHING),op(OP_NAME,$1)); }
        ;

expr_list   :	/*EMPTY*/
            |	{ many_arg=false; } expr_list0
            ;

expr_list0	:	expr	{ if(typeError) break;
                          arg=currentArg;
                          if(paramChecked(&many_arg,&currentArg,$1)) {
                            if(paramMode(arg)==PASS_BY_VALUE) {
                                temp=newTemp(paramType(arg));
                                genQuad(ASSIGN,op(OP_PLACE,$1.place),op(OP_NOTHING),op(OP_PLACE,temp));
                                genQuad(PAR,op(OP_PLACE,temp),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING));
                            }
                            else
                                genQuad(PAR,op(OP_PLACE,$1.place),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING));
                          }
                          if(currentArg!=NULL)  
                             error("Too few arguments at call"); }

            |   T_string    { if(typeError) break;
                              arg=currentArg;
                              if(paramString(&many_arg,&currentArg))
                                genQuad(PAR,op(OP_STRING,$1),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING));
                              if(currentArg!=NULL) 
                                error("Too few arguments at call"); }

            |	expr	    { if(typeError) break;
                              arg=currentArg;
                              if(!paramChecked(&many_arg,&currentArg,$1)) break;
                              if(paramMode(arg)==PASS_BY_VALUE) {
                                temp=newTemp(paramType(arg));
                                genQuad(ASSIGN,op(OP_PLACE,$1.place),op(OP_NOTHING),op(OP_PLACE,temp));
                                genQuad(PAR,op(OP_PLACE,temp),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING));
                              }
                              else
                                genQuad(PAR,op(OP_PLACE,$1.place),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING)); }
                                
                T_comma expr_list0
           |    T_string        { if(typeError) break;
                                  arg=currentArg;
                                 if(paramString(&many_arg,&currentArg)) 
                                    genQuad(PAR,op(OP_STRING,$1),op(OP_PASSMODE,paramMode(arg)),op(OP_NOTHING)); }
                T_comma expr_list0
           ;

expr    :	T_constnum	{ $$.type = typeInteger;
                          sprintf(buff,"%d",const_counter++);
                          $$.place.placeType = ENTRY;
                          $$.place.entry = newConstant(buff,typeInteger,$1); }

        |	T_constchar	{ $$.type = typeChar;
                          sprintf(buff,"%d",const_counter++);
                          $$.place.placeType = ENTRY;
                          $$.place.entry = newConstant(buff,typeChar,$1); }

        |	l_value		{ $$.type = $1.type;
                          $$.place = $1.place; }


        |	T_oppar expr T_clpar	{ $$.type = $2.type;
                                      $$.place = $2.place;}
       
        |	func_call	{ if($1.type!=typeVoid) 
                            $$ = $1;
                          else {
                            error("Cannot call proc");
                            $$.type = typeUnknown; 
                          } }

        |	T_plus expr %prec UPLUS	{ if(($2.type!=typeInteger)&&($2.type!=typeUnknown)) { 
                                        error("Opperand must be type int");
                                        $$.type = typeUnknown;
                                        break;
                                      }
                                      $$ = $2; }

        |	T_minus expr %prec UMINUS	{ if($2.type==typeUnknown) {
                                            $$.type = typeUnknown;
                                            break;
                                          }
                                          if($2.type!=typeInteger) {
                                            error("Opperand must be type int");
                                            $$.type = typeUnknown;
                                            break;
                                          }
                                          $$.type = $2.type;
                                          temp = newTemp($2.type);
                                          genQuad(MINUS,op(OP_PLACE,$2.place),op(OP_NOTHING),op(OP_PLACE,temp));
                                          $$.place = temp; }

        |	expr T_plus expr	{ binopQuad(PLUS, &($1), &($3), &($$)); }

        |	expr T_minus expr	{ binopQuad(MINUS, &($1), &($3), &($$)); }

        |	expr T_mult expr	{ binopQuad(MULT, &($1), &($3), &($$)); }

        |	expr T_div expr		{ binopQuad(DIVI, &($1), &($3), &($$)); }

        |	expr T_mod expr		{ binopQuad(MOD, &($1), &($3), &($$)); }

        ;


l_value	:	lval_id                     { $$ = $1; }
        |	lval_id T_opj expr T_clj    { if($3.type != typeInteger) {
                                            error("Array index must be integer");
                                            $$.type = typeUnknown;
                                            break;
                                          }
                                          if($1.type->kind != TYPE_ARRAY) {
                                             error("Identifier is not array");
                                             $$.type = typeUnknown;
                                             break;
                                          }
                                          $$.type = $1.type->refType;
                                          temp = newTemp(typePointer($$.type));
                                          genQuad(ARRAY,op(OP_PLACE,$1.place),op(OP_PLACE,$3.place),op(OP_PLACE,temp));
                                          $$.place.entry = temp.entry; }
        ;

lval_id : T_id  { if((lval = lookupEntry($1,LOOKUP_ALL_SCOPES,true))==NULL) {
                  /*  error("Identifier cannot be found"); */
                    newVariable($1,typeUnknown);
                    $$.type = typeUnknown;
                    break;
                  }
                  if(lval->entryType == ENTRY_VARIABLE) {
                    $$.type = lval->u.eVariable.type;
                    $$.place.placeType = ENTRY;
                    $$.place.entry = lval;
                    break;
                  }
                  if(lval->entryType == ENTRY_PARAMETER) {
                    $$.type = lval->u.eParameter.type;
                    $$.place.placeType = ENTRY;
                    $$.place.entry = lval;
                    break;
                  }
                  error("Identifier %s is not a valid variable",$1);
                  $$.type = typeUnknown; } 
                                               
        ;

cond    :	T_true  { genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
                      $$.True = makeList(currentQuad());
                      genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
                      $$.False = makeList(currentQuad()); }

        |	T_false { genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
                      $$.False = makeList(currentQuad());
                      genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
                      $$.True = makeList(currentQuad()); }

        |	T_oppar cond T_clpar { $$=$2; }

        |	T_excl cond     { $$.True = $2.False; $$.False = $2.True; }

        |	expr T_eq expr	{ relopQuad(EQ, &($1), &($3),&($$)); }

        |	expr T_ne expr	{ relopQuad(NEQ, &($1), &($3),&($$)); }

        |	expr T_gt expr	{ relopQuad(GT, &($1), &($3),&($$)); }

        |	expr T_lt expr	{ relopQuad(LT, &($1), &($3),&($$)); }

        |	expr T_ge expr	{ relopQuad(GE, &($1), &($3),&($$)); }

        |	expr T_le expr	{ relopQuad(LE, &($1), &($3),&($$)); }

        |	cond T_and { backpatch($1.True,nextQuad()); } 
            cond       { $$.True = $4.True; $$.False = merge($1.False,$4.False); }

        |	cond T_or { backpatch($1.False,nextQuad()); } 
            cond      {  $$.True = merge($1.True,$4.True); $$.False = $4.False; }
        ;

%%


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
