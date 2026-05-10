#include "optimize.h"
#include "tac.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_num(const char *s) {
    if (!s || !*s)
        return 0;
    char *end = NULL;
    strtod(s, &end);
    return end && *end == '\0';
}

static double num_val(const char *s) { return strtod(s, NULL); }

static void pass_constant_folding(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->arg1 || !in->arg2)
            continue;
        if (!is_num(in->arg1) || !is_num(in->arg2))
            continue;
        double a = num_val(in->arg1);
        double b = num_val(in->arg2);
        char buf[64];
        switch (in->op) {
        case TAC_OP_ADD:
            snprintf(buf, sizeof(buf), "%.6g", a + b);
            break;
        case TAC_OP_SUB:
            snprintf(buf, sizeof(buf), "%.6g", a - b);
            break;
        case TAC_OP_MUL:
            snprintf(buf, sizeof(buf), "%.6g", a * b);
            break;
        case TAC_OP_DIV:
            if (b == 0.0)
                continue;
            snprintf(buf, sizeof(buf), "%.6g", a / b);
            break;
        default:
            continue;
        }
        free(in->arg1);
        free(in->arg2);
        in->arg1 = strdup(buf);
        in->arg2 = NULL;
        in->op = TAC_OP_COPY;
    }
}

static void pass_dead_code_elimination(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->result)
            continue;
        if (in->op == TAC_OP_LABEL)
            continue;
        if (in->result[0] != 't')
            continue;
        int refs = 0;
        int seen_def = 0;
        for (TacInst *q = p->head; q; q = q->next) {
            if (q == in) {
                seen_def = 1;
                continue;
            }
            if (!seen_def)
                continue;
            if (q->arg1 && strcmp(q->arg1, in->result) == 0)
                refs++;
            if (q->arg2 && strcmp(q->arg2, in->result) == 0)
                refs++;
            if (q->result && strcmp(q->result, in->result) == 0 && q != in)
                break;
        }
        if (refs == 0 && in->op != TAC_OP_IF_EQ && in->op != TAC_OP_IF_NE && in->op != TAC_OP_GOTO)
            in->dead = true;
    }
}

static void prune_dead(TacProgram *p) {
    TacInst *cur = p->head;
    TacInst *prev = NULL;
    while (cur) {
        if (cur->dead) {
            TacInst *nx = cur->next;
            if (prev)
                prev->next = nx;
            else
                p->head = nx;
            if (p->tail == cur)
                p->tail = prev;
            free(cur->result);
            free(cur->arg1);
            free(cur->arg2);
            free(cur);
            cur = nx;
            continue;
        }
        prev = cur;
        cur = cur->next;
    }
}

static void pass_unreachable(TacProgram *p) {
    /* Mark instructions after unconditional goto until next label */
    int skip = 0;
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op == TAC_OP_LABEL) {
            skip = 0;
            continue;
        }
        if (skip)
            in->dead = true;
        if (in->op == TAC_OP_GOTO)
            skip = 1;
    }
    prune_dead(p);
}

static void pass_constant_propagation(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op != TAC_OP_COPY || !in->result || in->result[0] != 't')
            continue;
        if (!is_num(in->arg1))
            continue;
        const char *val = in->arg1;
        for (TacInst *q = in->next; q; q = q->next) {
            if (q->arg1 && strcmp(q->arg1, in->result) == 0 && is_num(val)) {
                free(q->arg1);
                q->arg1 = strdup(val);
            }
            if (q->arg2 && strcmp(q->arg2, in->result) == 0 && is_num(val)) {
                free(q->arg2);
                q->arg2 = strdup(val);
            }
            if (q->result && strcmp(q->result, in->result) == 0 && q != in)
                break;
        }
    }
}

static void pass_cse(TacProgram *p) {
    /* Reuse identical binary ops on same operands */
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op != TAC_OP_ADD && in->op != TAC_OP_MUL)
            continue;
        if (!in->arg1 || !in->arg2 || !in->result)
            continue;
        for (TacInst *q = p->head; q && q != in; q = q->next) {
            if (q->op != in->op)
                continue;
            if (!q->result || q->result[0] != 't')
                continue;
            if (strcmp(q->arg1 ? q->arg1 : "", in->arg1 ? in->arg1 : "") != 0)
                continue;
            if (strcmp(q->arg2 ? q->arg2 : "", in->arg2 ? in->arg2 : "") != 0)
                continue;
            free(in->arg1);
            free(in->arg2);
            in->arg1 = strdup(q->result);
            in->arg2 = NULL;
            in->op = TAC_OP_COPY;
            break;
        }
    }
}

TacProgram *optimize_tac(TacProgram *p) {
    if (!p)
        return NULL;
    printf("\n=== Module 7 — Technique 1: Constant propagation ===\n");
    printf("(Replace uses of temps that hold a compile-time constant.)\n");
    pass_constant_propagation(p);
    tac_print(p);
    printf("\n=== Module 7 — Technique 2: Constant folding ===\n");
    printf("(Evaluate binary ops on constant operands at compile time.)\n");
    pass_constant_folding(p);
    tac_print(p);
    printf("\n=== Module 7 — Technique 3: Common subexpression elimination (CSE) ===\n");
    printf("(Reuse result of identical operations on same operands.)\n");
    pass_cse(p);
    tac_print(p);
    printf("\n=== Module 7 — Task 4: Unreachable code removal ===\n");
    printf("(Delete instructions after unconditional goto until next label.)\n");
    pass_unreachable(p);
    tac_print(p);
    printf("\n=== Module 7 — Technique 5: Dead code elimination ===\n");
    printf("(Remove unused temp definitions that do not affect control flow.)\n");
    pass_dead_code_elimination(p);
    prune_dead(p);
    tac_print(p);
    return p;
}
