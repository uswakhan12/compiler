/*
 * Module 7 — Technique 5: Dead Code Elimination.
 *
 * Conservative whole-program DCE: a temp definition is dead iff NO
 * instruction anywhere in the program reads it.  We deliberately keep
 * ops with side effects (calls, returns, params, array stores) even
 * when their result is unused.
 *
 * A flow-sensitive DCE could remove more (e.g. dead defs along one
 * branch of an `if`), but that would require reaching-definitions
 * analysis.  The whole-program form is sound under arbitrary control
 * flow.
 */

#include "opt_passes.h"
#include <string.h>

void pass_dead_code_elimination(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->result)
            continue;
        if (in->op == TAC_OP_LABEL)
            continue;
        if (in->result[0] != 't')
            continue;
        if (in->op == TAC_OP_CALL || in->op == TAC_OP_PARAM ||
            in->op == TAC_OP_RETURN || in->op == TAC_OP_ARR_STORE)
            continue;
        int refs = 0;
        for (TacInst *q = p->head; q; q = q->next) {
            if (q == in)
                continue;
            if (q->arg1 && strcmp(q->arg1, in->result) == 0)
                refs++;
            if (q->arg2 && strcmp(q->arg2, in->result) == 0)
                refs++;
        }
        if (refs == 0 && in->op != TAC_OP_IF_EQ && in->op != TAC_OP_IF_NE &&
            in->op != TAC_OP_IF_LT && in->op != TAC_OP_IF_GT &&
            in->op != TAC_OP_IF_LE && in->op != TAC_OP_IF_GE &&
            in->op != TAC_OP_GOTO)
            in->dead = true;
    }
    opt_prune_dead(p);
}
