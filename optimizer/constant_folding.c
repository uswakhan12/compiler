/*
 * Module 7 — Technique 2: Constant Folding.
 *
 * Walk the TAC list and, for every binary op whose two operands are
 * literal numbers, evaluate the operation at compile time and replace
 * the instruction with a copy of the result.  e.g.
 *
 *     t0 = 3 + 4         -->         t0 = 7
 *
 * Division by zero is detected and skipped (sound), so the program's
 * runtime behaviour is preserved.
 */

#include "opt_passes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pass_constant_folding(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->arg1 || !in->arg2)
            continue;
        if (!opt_is_num(in->arg1) || !opt_is_num(in->arg2))
            continue;
        double a = opt_num_val(in->arg1);
        double b = opt_num_val(in->arg2);
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
