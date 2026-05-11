/*
 * Module 7 — Task 4: Unreachable Code Removal.
 *
 * After an unconditional control transfer (`goto L` or `return x`) the
 * remaining instructions in the current basic block are unreachable
 * until the next label re-enters control flow.  We mark them dead and
 * splice them out via the shared opt_prune_dead helper.
 *
 * This complements DCE: unreachable code may have side effects (calls,
 * stores) that DCE would conservatively keep, so structurally removing
 * it first lets the rest of the pipeline see a tighter CFG.
 */

#include "opt_passes.h"
#include <stdio.h>

void pass_unreachable(TacProgram *p) {
    int killed = 0;
    int skip = 0;
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op == TAC_OP_LABEL) {
            skip = 0;
            continue;
        }
        if (skip) {
            in->dead = true;
            killed++;
        }
        if (in->op == TAC_OP_GOTO || in->op == TAC_OP_RETURN)
            skip = 1;
    }
    if (killed == 0)
        printf("    Unreachable: no dead-after-jump/return code found.\n");
    else
        printf("    Unreachable: removed %d instruction(s) after goto/return.\n", killed);
    opt_prune_dead(p);
}
