/*
 * Module 7 — Technique 2b: Algebraic Simplification.
 *
 * Identity rewrites that hold for both int and float operands:
 *
 *   x + 0   -> x         0 + x  -> x
 *   x - 0   -> x
 *   x * 1   -> x         1 * x  -> x
 *   x * 0   -> 0         0 * x  -> 0
 *   x / 1   -> x
 *
 * Each rewrite converts a binary op into either a copy or a constant,
 * which becomes a candidate for downstream dead-code elimination if
 * the result is never read.
 *
 * Note: x / 0 is NOT rewritten — preserving the runtime trap is
 * required for soundness.
 */

#include "opt_passes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pass_algebraic_simplification(TacProgram *p) {
    int rewrites = 0;
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->arg1 || !in->arg2)
            continue;
        int a_is_num = opt_is_num(in->arg1);
        int b_is_num = opt_is_num(in->arg2);
        double a = a_is_num ? opt_num_val(in->arg1) : 0;
        double b = b_is_num ? opt_num_val(in->arg2) : 0;
        int hit = 0;
        const char *what = "";
        switch (in->op) {
        case TAC_OP_ADD:
            if (b_is_num && b == 0) {
                free(in->arg2); in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "x + 0 -> x";
            } else if (a_is_num && a == 0) {
                free(in->arg1); in->arg1 = in->arg2; in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "0 + x -> x";
            }
            break;
        case TAC_OP_SUB:
            if (b_is_num && b == 0) {
                free(in->arg2); in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "x - 0 -> x";
            }
            break;
        case TAC_OP_MUL:
            if ((a_is_num && a == 0) || (b_is_num && b == 0)) {
                free(in->arg1); free(in->arg2);
                in->arg1 = strdup("0"); in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "x * 0 -> 0";
            } else if (b_is_num && b == 1) {
                free(in->arg2); in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "x * 1 -> x";
            } else if (a_is_num && a == 1) {
                free(in->arg1); in->arg1 = in->arg2; in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "1 * x -> x";
            }
            break;
        case TAC_OP_DIV:
            if (b_is_num && b == 1) {
                free(in->arg2); in->arg2 = NULL;
                in->op = TAC_OP_COPY; hit = 1; what = "x / 1 -> x";
            }
            break;
        default:
            break;
        }
        if (hit) {
            printf("    Algebraic: %s  on `%s`\n", what, in->result ? in->result : "_");
            rewrites++;
        }
    }
    if (rewrites == 0)
        printf("    Algebraic: no identities applicable.\n");
}
