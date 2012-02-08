#include "symbol.h"

typedef enum {
        UNIT, ENDU,
        PLUS, MINUS, MULT, DIVI, MOD,
        ASSIGN, ARRAY,
        EQ, NEQ, LT, GT, LE, GE,
        IFB, JUMP, LABEL, JUMPL,
        CALL, PAR, RET
} oper;

typedef struct operand_tag operand;

struct operand_tag {
        enum {
                OP_ENTRY,
                OP_LABEL,
                OP_PASSMODE,
                OP_UNKNOWN,
                OP_NOTHING
        } opType;

        union {
                int label;
                SymbolEntry * symb;
        }u;
};

typedef struct quadListNode_tag quadListNode;

struct quadListNode_tag {
        int quadLabel;
        oper op; 
        operand *operand0; 
        operand *operand1;
        operand *operand2;
        quadListNode* next;
};

typedef struct labelList_tag labelList;

struct labelList_tag {
        quadListNode* label;
        labelList* next;
};

quadListNode *quadFirst, *quadLast;

int nextQuad();

void genQuad(oper a, operand *b, operand *c, operand *d);

SymbolEntry * newTemp(Type t);

labelList * emptyList();

labelList * makeList(int x);

labelList * merge(labelList * l1, labelList * l2);
