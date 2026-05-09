#ifndef TAC_H
#define TAC_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TAC_OP_ADD,
    TAC_OP_SUB,
    TAC_OP_MUL,
    TAC_OP_DIV,
    TAC_OP_POW,
    TAC_OP_NEG,
    TAC_OP_ASSIGN,
    TAC_OP_COPY,
    TAC_OP_GOTO,
    TAC_OP_IF_LT,
    TAC_OP_IF_GT,
    TAC_OP_IF_LE,
    TAC_OP_IF_GE,
    TAC_OP_IF_EQ,
    TAC_OP_IF_NE,
    TAC_OP_LABEL,
    TAC_OP_PARAM,
    TAC_OP_CALL,
    TAC_OP_LOG,
    TAC_OP_EXP,
    TAC_OP_NOP,
    TAC_OP_CAST_INT_TO_FLOAT
} TacOp;

typedef struct TacInst TacInst;
struct TacInst {
    TacOp op;
    char *result;
    char *arg1;
    char *arg2;
    char *extra;
    TacInst *next;
    TacInst *prev;
    int id;
    bool dead;
};

typedef struct TacProgram TacProgram;
struct TacProgram {
    TacInst *head;
    TacInst *tail;
    int next_label;
    int next_temp;
};

TacProgram *tac_new(void);
void tac_emit(TacProgram *p, TacOp op, const char *r, const char *a1, const char *a2);
char *tac_new_temp(TacProgram *p);
char *tac_new_label(TacProgram *p);
void tac_free_program(TacProgram *p);
void tac_print(const TacProgram *p);

#endif
