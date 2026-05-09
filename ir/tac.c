#include "tac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *op_str(TacOp op) {
    switch (op) {
    case TAC_OP_ADD:
        return "+";
    case TAC_OP_SUB:
        return "-";
    case TAC_OP_MUL:
        return "*";
    case TAC_OP_DIV:
        return "/";
    case TAC_OP_POW:
        return "^";
    case TAC_OP_NEG:
        return "neg";
    case TAC_OP_ASSIGN:
        return "=";
    case TAC_OP_COPY:
        return ":=";
    case TAC_OP_GOTO:
        return "goto";
    case TAC_OP_IF_LT:
        return "if<";
    case TAC_OP_IF_GT:
        return "if>";
    case TAC_OP_IF_LE:
        return "if<=";
    case TAC_OP_IF_GE:
        return "if>=";
    case TAC_OP_IF_EQ:
        return "if==";
    case TAC_OP_IF_NE:
        return "if!=";
    case TAC_OP_LABEL:
        return "label";
    case TAC_OP_PARAM:
        return "param";
    case TAC_OP_CALL:
        return "call";
    case TAC_OP_LOG:
        return "log";
    case TAC_OP_EXP:
        return "exp";
    case TAC_OP_CAST_INT_TO_FLOAT:
        return "int2float";
    default:
        return "?";
    }
}

TacProgram *tac_new(void) {
    TacProgram *p = (TacProgram *)calloc(1, sizeof(TacProgram));
    p->next_label = 0;
    p->next_temp = 0;
    return p;
}

void tac_emit(TacProgram *p, TacOp op, const char *r, const char *a1, const char *a2) {
    if (!p)
        return;
    TacInst *in = (TacInst *)calloc(1, sizeof(TacInst));
    in->op = op;
    in->result = r ? strdup(r) : NULL;
    in->arg1 = a1 ? strdup(a1) : NULL;
    in->arg2 = a2 ? strdup(a2) : NULL;
    in->id = p->head ? p->tail->id + 1 : 0;
    if (!p->head) {
        p->head = p->tail = in;
        in->prev = NULL;
    } else {
        in->prev = p->tail;
        p->tail->next = in;
        p->tail = in;
    }
}

char *tac_new_temp(TacProgram *p) {
    char buf[32];
    snprintf(buf, sizeof(buf), "t%d", p->next_temp++);
    return strdup(buf);
}

char *tac_new_label(TacProgram *p) {
    char buf[32];
    snprintf(buf, sizeof(buf), "L%d", p->next_label++);
    return strdup(buf);
}

void tac_free_program(TacProgram *p) {
    if (!p)
        return;
    TacInst *in = p->head;
    while (in) {
        TacInst *nx = in->next;
        free(in->result);
        free(in->arg1);
        free(in->arg2);
        free(in->extra);
        free(in);
        in = nx;
    }
    free(p);
}

void tac_print(const TacProgram *p) {
    if (!p || !p->head) {
        printf("(empty TAC)\n");
        return;
    }
    for (TacInst *in = p->head; in; in = in->next) {
        printf("[%d] ", in->id);
        switch (in->op) {
        case TAC_OP_LABEL:
            printf("%s:\n", in->result ? in->result : "");
            break;
        case TAC_OP_GOTO:
            printf("goto %s\n", in->result ? in->result : "");
            break;
        case TAC_OP_IF_EQ:
            printf("if %s == %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_IF_NE:
            printf("if %s != %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_IF_LT:
            printf("if %s < %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_IF_GT:
            printf("if %s > %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_IF_LE:
            printf("if %s <= %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_IF_GE:
            printf("if %s >= %s goto %s\n", in->arg1 ? in->arg1 : "_", in->arg2 ? in->arg2 : "_",
                   in->result ? in->result : "_");
            break;
        case TAC_OP_CALL:
            printf("%s = call %s\n", in->result ? in->result : "_", in->arg1 ? in->arg1 : "");
            break;
        case TAC_OP_PARAM:
            printf("param %s\n", in->arg1 ? in->arg1 : "");
            break;
        case TAC_OP_LOG:
        case TAC_OP_EXP:
            printf("%s = %s(%s)\n", in->result ? in->result : "_", op_str(in->op),
                   in->arg1 ? in->arg1 : "");
            break;
        case TAC_OP_COPY:
        case TAC_OP_ASSIGN:
            printf("%s = %s\n", in->result ? in->result : "_", in->arg1 ? in->arg1 : "");
            break;
        case TAC_OP_CAST_INT_TO_FLOAT:
            printf("%s = (float)%s\n", in->result ? in->result : "_", in->arg1 ? in->arg1 : "");
            break;
        default:
            if (in->arg2)
                printf("%s = %s %s %s\n", in->result ? in->result : "_", in->arg1 ? in->arg1 : "",
                       op_str(in->op), in->arg2 ? in->arg2 : "");
            else if (in->op == TAC_OP_NEG)
                printf("%s = -%s\n", in->result ? in->result : "_", in->arg1 ? in->arg1 : "");
            else
                printf("%s %s %s\n", op_str(in->op), in->arg1 ? in->arg1 : "", in->arg2 ? in->arg2 : "");
            break;
        }
    }
}
