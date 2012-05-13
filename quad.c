#include "quad.h"
#include <stdio.h>
#include <stdlib.h>

unsigned int quadNext= 0;
quadListNode * quadFirst = NULL;
quadListNode * quadLast = NULL;

int nextQuad()
{
    return quadNext;
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
