/*
 * Shared utilities used by every optimisation pass.
 *
 *   opt_is_num / opt_num_val   numeric-literal detection (TAC operands
 *                              are strings; we only fold/propagate ones
 *                              that fully parse as a number).
 *   opt_prune_dead             splice out every TacInst whose `dead`
 *                              flag is set, freeing its storage.
 *
 * The split-out helpers let every pass be its own translation unit
 * (Module 7 Task 1 "Refactoring").
 */

#include "opt_passes.h"
#include <stdlib.h>
#include <string.h>

int opt_is_num(const char *s) {
    if (!s || !*s)
        return 0;
    char *end = NULL;
    strtod(s, &end);
    return end && *end == '\0';
}

double opt_num_val(const char *s) { return strtod(s, NULL); }

void opt_prune_dead(TacProgram *p) {
    TacInst *cur = p->head;
    TacInst *prev = NULL;
    while (cur) {
        if (cur->dead) {
            TacInst *nx = cur->next;
            if (prev)
                prev->next = nx;
            else
                p->head = nx;
            if (nx)
                nx->prev = prev;
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
