#ifndef QUAD_H
#define QUAD_H 0

#include "symbol.h"


typedef enum {
    UNIT, ENDU,
    PLUS, MINUS, MULT, DIVI, MOD,
    ASSIGN, ARRAY,
    EQ, NEQ, LT, GT, LE, GE,
    IFB, JUMP, LABEL, JUMPL,
    CALL, PAR, RET
} oper;

typedef struct Place_tag Place;

struct Place_tag {
    enum {
        CONSTNUM,
        CONSTCHAR,
        ENTRY
    } placeType;

    union {
        int constnum;
        char constchar;
        SymbolEntry *entry;
    }u;
};

typedef struct operand_tag operand;

struct operand_tag {
    enum {
        OP_PLACE,
        OP_TEMPORARY,
        OP_LABEL,
        OP_PASSMODE,
        OP_UNKNOWN,
        OP_NOTHING
    } opType;

    union {
        SymbolEntry *temp;
        int label;
        Place place;
    }u;
};

typedef struct quadListNode_tag quadListNode;

struct quadListNode_tag {
    int quadLabel;

    oper op; 
    operand operand0; 
    operand operand1;
    operand operand2;
    quadListNode* next;
};

typedef struct labelList_tag labelList;

struct labelList_tag {
    quadListNode* label;
    labelList* next;
};

quadListNode *quadFirst, *quadLast;

int nextQuad();

void genQuad(oper a, operand b, operand c, operand d);

SymbolEntry * newTemp(Type t);

labelList * emptyList();

labelList * makeList(int x);

labelList * merge(labelList * l1, labelList * l2);

void printQuads();
#endif
