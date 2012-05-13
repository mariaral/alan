#ifndef QUAD_H
#define QUAD_H 0

#include "symbol.h"


typedef enum {
    UNIT, ENDU,
    PLUS, MINUS, MULT, DIVI, MOD,
    ASSIGN, ARRAY,
    EQ, NEQ, LT, GT, LE, GE,
    IFB, JUMP, JUMPL,
    CALL, PAR, RET
} oper;

typedef enum {
    OP_PLACE,
    OP_LABEL,
    OP_NAME,
    OP_PASSMODE,
    OP_UNKNOWN,
    OP_NOTHING
} op_type;

typedef struct Place_tag Place;

struct Place_tag {
    enum {
        ENTRY,
        REFERENCE
    } placeType;
    SymbolEntry * entry;
}; 
        
typedef struct operand_tag operand;

struct operand_tag {
    op_type opType;
    union {
        int label;
        Place place;
        char name[256];
    }u;
};

typedef struct varstr_tag varstr;

struct varstr_tag {
    Place place;
    Type type;
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

void genQuad(oper, operand, operand, operand);

/*operand address_of(operand);*/

Place newTemp(Type);

labelList * emptyList();

labelList * makeList(int);

labelList * merge(labelList * , labelList * );

void backpatch(labelList *, int);
    
void printQuads();

void printOp(operand);
#endif
