/*
 * Module 7 — Technique 1: Constant Propagation.
 *
 * Whenever a temporary holds a compile-time constant (i.e. a `t = N`
 * copy where N is numeric), forward the constant into every subsequent
 * use of t within the SAME basic block.
 *
 * Soundness: propagation stops at labels and `goto` instructions
 * because a value at a label may be the meet of multiple definitions
 * on different control-flow paths (e.g. the two branches of an `if`).
 * It also stops on any redefinition of the temp.  Without a full
 * reaching-definitions analysis this conservative form is the safe
 * lower bound that still demonstrates the technique.
 */

#include "opt_passes.h"
#include <stdlib.h>
#include <string.h>

void pass_constant_propagation(TacProgram *p) {
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op != TAC_OP_COPY || !in->result || in->result[0] != 't')
            continue;
        if (!opt_is_num(in->arg1))
            continue;
        const char *val = in->arg1;
        for (TacInst *q = in->next; q; q = q->next) {
            if (q->op == TAC_OP_LABEL || q->op == TAC_OP_GOTO)
                break;
            if (q->arg1 && strcmp(q->arg1, in->result) == 0) {
                free(q->arg1);
                q->arg1 = strdup(val);
            }
            if (q->arg2 && strcmp(q->arg2, in->result) == 0) {
                free(q->arg2);
                q->arg2 = strdup(val);
            }
            if (q->result && strcmp(q->result, in->result) == 0 && q != in)
                break;
        }
    }
}
