#include "quad.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
/*
   operand address_of(operand x)
   {
   operand r;

   r.opType = OP_PLACE;
   r.u.place = 
   } 
   */
void genQuad(oper a, operand b, operand c, operand d)
{
    quadListNode* newQuad;

    newQuad = malloc(sizeof(quadListNode));
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

    p = malloc(sizeof(labelList));
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
        default:
            break;
    }
    return op;

}

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

void binopQuad(oper opr, varstr *exp1, varstr *exp2, varstr *ret)
{
    Place temp;

    if(!int_or_byte(exp1->type,exp2->type))
        error("Operands must be type int or byte");
    else {
        if(exp1->type != exp2->type)
            error("Operands must be same type");
        else {
            ret->type = exp1->type;
            temp = newTemp(exp2->type);
            genQuad(opr,op(OP_PLACE,exp1->place),op(OP_PLACE,exp2->place),op(OP_PLACE,temp));
            ret->place = temp;
        }
    }
}

void relopQuad(oper opr, varstr *exp1, varstr *exp2, boolean *ret)
{
    if(!int_or_byte(exp1->type,exp2->type))
        error("Opperands must be int or byte");
    else if(exp1->type!=exp2->type)
        error("Cannot compaire different types");
    genQuad(opr, op(OP_PLACE,exp1->place),op(OP_PLACE,exp2->place),op(OP_UNKNOWN));
    ret->True = makeList(currentQuad());
    genQuad(JUMP,op(OP_NOTHING),op(OP_NOTHING),op(OP_UNKNOWN));
    ret->False = makeList(currentQuad());
}


void printQuads()
{
    quadListNode * temp;

    temp = quadFirst;
    while (temp != NULL) {
        printf("%d: ",temp->quadLabel);
        switch (temp->op) {
            case UNIT:
                printf("unit, ");
                break;
            case ENDU:
                printf("endu, ");
                break;
            case PLUS:
                printf("+, ");
                break;
            case MINUS:
            printf("-, ");
            break;
        case MULT:
            printf("*, ");
            break;
        case DIVI:
            printf("/, ");
            break;
        case MOD:
            printf("%%, ");
            break;
        case ASSIGN:
            printf(":=, ");
            break;
        case ARRAY:
            printf("array, ");
            break;
        case EQ:
            printf("=, ");
            break;
        case NEQ:
            printf("<>, ");
            break;
        case LT:
            printf("<, ");
            break;
        case GT:
            printf(">, ");
            break;
        case LE:
            printf("<=, ");
            break;
        case GE:
            printf(">=, ");
            break;
        case JUMP:
            printf("jump, ");
            break;
        }
        printOp(temp->operand0);
        printf(", ");
        printOp(temp->operand1);
        printf(", ");
        printOp(temp->operand2);
        printf("\n");
        temp = temp->next;
    }
}

void printOp(operand op)
{
    Place p;
    
    switch (op.opType) {
    case OP_LABEL:
        printf("%d",op.u.label);
        break;
    case OP_NAME:
        printf("%s",op.u.name);
        break;
    case OP_UNKNOWN:
        printf("*");
        break;
    case OP_NOTHING:
        printf("-");
        break;
    case OP_PLACE:
        p = op.u.place;
        switch (p.entry->entryType) {
        case ENTRY_VARIABLE:
            printf("%s",p.entry->id);
            break;
        case ENTRY_CONSTANT:
            if(p.entry->u.eConstant.type == typeInteger)
                printf("%d",p.entry->u.eConstant.value.vInteger);
            else printf("'%c'",p.entry->u.eConstant.value.vChar);
            break;
        case ENTRY_TEMPORARY:
            if(p.placeType == REFERENCE)
                printf("[$%d]",p.entry->u.eTemporary.number);
            else 
                printf("$%d",p.entry->u.eTemporary.number);
            
        }
    }
}
