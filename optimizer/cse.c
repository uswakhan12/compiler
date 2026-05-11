/*
 * Module 7 — Technique 3: Common Sub-Expression Elimination (CSE).
 *
 * If two identical binary ops over the same operands appear within the
 * same basic block, the second one is rewritten as a copy of the first
 * result.  We restrict CSE to within a basic block (no cross-label
 * matches) because soundly extending across control flow requires
 * reaching-definitions analysis that we deliberately do not implement.
 *
 * Only commutative-friendly ops (ADD, MUL) are matched; the order of
 * operands must match exactly so we never produce wrong results for
 * non-commutative operations.
 */

#include "opt_passes.h"
#include <stdlib.h>
#include <string.h>

void pass_cse(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op != TAC_OP_ADD && in->op != TAC_OP_MUL)
            continue;
        if (!in->arg1 || !in->arg2 || !in->result)
            continue;
        /* Find the start of the basic block containing `in`. */
        TacInst *bb_start = p->head;
        for (TacInst *q = p->head; q && q != in; q = q->next)
            if (q->op == TAC_OP_LABEL)
                bb_start = q;
        for (TacInst *q = bb_start; q && q != in; q = q->next) {
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
