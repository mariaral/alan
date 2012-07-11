#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "general.h"
#include "quad.h"
#include "error.h"
#include "typecheck.h"

extern FILE *outfile;
extern bool pquads;

unsigned int quadNext= 0;
quadListNode * quadFirst = NULL;
quadListNode * quadLast = NULL;

int nextQuad()
{
    return quadNext;
}

int currentQuad()
{
    return (quadNext-1);
}

void genQuad(oper a, operand b, operand c, operand d)
{
    quadListNode* newQuad;

    newQuad = new(sizeof(quadListNode));
    newQuad->op = a;
    newQuad->operand0 = b;
    newQuad->operand1 = c;
    newQuad->operand2 = d;
    newQuad->quadLabel = quadNext++;
    newQuad->next = NULL;
    if (quadLast!=NULL)
        quadLast->next = newQuad;
    else quadFirst = newQuad;
    quadLast = newQuad;
}

Place newTemp(Type type)
{
    Place temp;
    temp.placeType = ENTRY;
    temp.entry = newTemporary(type);
    return temp;
}

labelList * emptyList()
{
    return NULL;
}

labelList * makeList(int x)
{
    quadListNode *temp;
    labelList *p;

    p = new(sizeof(labelList));
    temp = quadFirst;
    while((temp!=NULL)&&(temp->quadLabel!=x))
        temp = temp->next;
    p->label = temp;
    p->next = NULL;
    return p;
}

labelList * merge(labelList * l1, labelList * l2)
{
    labelList *temp;
    temp = l1;
    if(temp==NULL)
        return l2;
    while(temp->next!= NULL)
        temp = temp->next;
    temp->next = l2;
    return l1;
}

void backpatch(labelList * l, int z)
{
    labelList * temp;

    temp = l;
    while(temp!=NULL) {

        if (temp->label->operand2.opType==OP_UNKNOWN) {
            temp->label->operand2.opType = OP_LABEL;
            temp->label->operand2.u.label = z;
        }
        temp = temp->next;
    }
}

operand op(op_type optype, ...)
{
    va_list ap;
    operand op;

    op.opType = optype;
    va_start(ap, optype);
    switch (optype){
        case OP_PLACE:
            op.u.place = va_arg(ap, Place);
            break;
        case OP_LABEL:
            op.u.label = va_arg(ap, int);
            break;
        case OP_NAME:
            strcpy(op.u.name, va_arg(ap, char*));
            break;
        case OP_STRING:
            strcpy(op.u.name, va_arg(ap, char*));
            break;
        case OP_PASSMODE:
            op.u.passmode = va_arg(ap, PassMode);
            break;
        default:
            break;
    }
    return op;

}
void binopQuad(oper opr, varstr *exp1, varstr *exp2, varstr *ret)
{
    Place temp;
    if(unknownType(exp1->type,exp2->type)) {
        ret->type = typeUnknown;
        return;
    }
    if(!int_or_byte(exp1->type,exp2->type)) {
        error("Operands must be type int or byte");
        ret->type = typeUnknown;
        return;
    }
    if(exp1->type != exp2->type) {
        error("Operands must be same type");
        ret->type = typeUnknown;
        return;
    }
    ret->type = exp1->type;
    temp = newTemp(exp2->type);
    genQuad(opr,op(OP_PLACE,exp1->place),op(OP_PLACE,exp2->place),op(OP_PLACE,temp));
    ret->place = temp;
}



void relopQuad(oper opr, varstr *exp1, varstr *exp2, boolean *ret)
{
    if(unknownType(exp1->type,exp2->type))
        goto out;
    if(!int_or_byte(exp1->type,exp2->type)) {
        error("Opperands must be int or byte");
        goto out;
    }
    if(exp1->type!=exp2->type) {
        error("Cannot compaire different types");
        goto out;
    }
    genQuad(opr, op(OP_PLACE,exp1->place),op(OP_PLACE,exp2->place),op(OP_UNKNOWN));
out:    
    ret->True = makeList(currentQuad());
    genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
    ret->False = makeList(currentQuad());
}

PassMode paramMode(SymbolEntry* arg)
{
    return (arg->u.eParameter.mode);
}

void printQuads()
{
    quadListNode * temp;

    temp = quadFirst;
    if(!pquads) return;
    while (temp != NULL) {
        fprintf(outfile,"%d: ",temp->quadLabel);
        switch (temp->op) {
        case UNIT:
            fprintf(outfile,"unit, ");
            break;
        case ENDU:
            fprintf(outfile,"endu, ");
            break;
        case PLUS:
            fprintf(outfile,"+, ");
            break;
        case MINUS:
            fprintf(outfile,"-, ");
            break;
        case MULT:
            fprintf(outfile,"*, ");
            break;
        case DIVI:
            fprintf(outfile,"/, ");
            break;
        case MOD:
            fprintf(outfile,"%%, ");
            break;
        case ASSIGN:
            fprintf(outfile,":=, ");
            break;
        case ARRAY:
            fprintf(outfile,"array, ");
            break;
        case EQ:
            fprintf(outfile,"=, ");
            break;
        case NEQ:
            fprintf(outfile,"<>, ");
            break;
        case LT:
            fprintf(outfile,"<, ");
            break;
        case GT:
            fprintf(outfile,">, ");
            break;
        case LE:
            fprintf(outfile,"<=, ");
            break;
        case GE:
            fprintf(outfile,">=, ");
            break;
        case JUMP:
            fprintf(outfile,"jump, ");
            break;
       case RET:
            fprintf(outfile,"ret, ");
            break;
       case PAR:
            fprintf(outfile,"par, ");
            break;
       case CALL:
            fprintf(outfile,"call, ");
            break;
        }
        printOp(temp->operand0);
        fprintf(outfile,", ");
        printOp(temp->operand1);
        fprintf(outfile,", ");
        printOp(temp->operand2);
        fprintf(outfile,"\n");
        temp = temp->next;
    }
}

void printOp(operand op)
{
    Place p;
    
    switch (op.opType) {
    case OP_LABEL:
        fprintf(outfile,"%d",op.u.label);
        break;
    case OP_NAME:
        fprintf(outfile,"%s",op.u.name);
        break;
    case OP_UNKNOWN:
        fprintf(outfile,"*");
        break;
    case OP_NOTHING:
        fprintf(outfile,"-");
        break;
    case OP_RESULT:
        fprintf(outfile,"RET");
        break;
    case OP_STRING:
        fprintf(outfile,"%s",op.u.name);
        break;
    case OP_PASSMODE:
        if(op.u.passmode==PASS_BY_VALUE)
            fprintf(outfile,"V");
        else fprintf(outfile,"R");
        break;
    case OP_PLACE:
        p = op.u.place;
        switch (p.entry->entryType) {
        case ENTRY_VARIABLE:
            fprintf(outfile,"%s",p.entry->id);
            break;
        case ENTRY_PARAMETER:
            fprintf(outfile,"%s",p.entry->id);
            break;
        case ENTRY_CONSTANT:
            if(p.entry->u.eConstant.type == typeInteger)
                fprintf(outfile,"%d",p.entry->u.eConstant.value.vInteger);
            else fprintf(outfile,"%s",p.entry->u.eConstant.value.vChar);
            break;
        case ENTRY_TEMPORARY:
            if(p.placeType == REFERENCE)
                fprintf(outfile,"[$%d]",p.entry->u.eTemporary.number);
            else 
                fprintf(outfile,"$%d",p.entry->u.eTemporary.number);
            break;
        default:
            fprintf(outfile,"Unknown operand");
            break;
        }
    }
}

SymbolEntry *newString(char *str)
{
    Type typeString;

    typeString = (struct Type_tag*) new(sizeof(struct Type_tag));
    typeString->kind = TYPE_ARRAY;
    typeString->refType = typeChar;
    typeString->size = strlen(str);
    typeString->refCount = 0;

    return newConstant(NULL, typeString, str);
}
