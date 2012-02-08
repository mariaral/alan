#include "quad.h"
#include <stdio.h>
#include <stdlib.h>

int quadNext = 0;
quadFirst = NULL;
quadLast = NULL;

int nextQuad()
{
        printf("next quad: %d\n",quadNext);
        return quadNext;
}

void genQuad(oper a, operand *b, operand *c, operand *d)
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
       printf("%d\n",quadLast->id);
}

SymbolEntry * newTemp(Type type)
{
        return (newTemporary(type));
}

labelList * emptyList()
{
        labelList *p;

        p = malloc(sizeof(labelList));
        p->label = NULL;
        p->next = NULL;
        return p;
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

                if (temp->label->operand2->opType==OP_UNKNOWN) {
                        temp->label->operand2->opType = OP_LABEL;
                        temp->label->operand2->u.qlabel = z;
                }
                temp = temp->next;
        }
}


