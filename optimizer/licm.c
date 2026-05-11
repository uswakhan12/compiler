/*
 * Module 7 — Task 5: Loop-Invariant Code Motion (LICM).
 *
 * Our `while` codegen always emits:
 *
 *     Lstart:
 *       ... cond ...
 *       if cond == 0 goto Lend
 *       ... body ...
 *       goto Lstart
 *     Lend:
 *
 * We detect natural loops by scanning for a `goto LABEL` whose target
 * appears earlier in the instruction list (a back-edge).  The
 * instructions in [target_label, back_goto) form a single-block loop
 * body.
 *
 * An instruction inside the loop is loop-invariant when:
 *   * its operands are either numeric literals or names whose
 *     definitions all lie OUTSIDE the loop body, AND
 *   * the result temp is defined exactly once inside the loop.
 *
 * The implementation iterates to a fix point because hoisting one
 * statement can make a subsequent statement invariant.  Two kinds of
 * statements get hoisted:
 *
 *   * binary ops (`t = a op b`)   — the classic LICM case
 *   * copies     (`t = src`)      — often produced by constant folding
 *                                    or algebraic simplification
 */

#include "opt_passes.h"
#include "optimize.h" /* public licm_optimize prototype */
#include <stdio.h>
#include <string.h>

static int is_binop(TacOp op) {
    return op == TAC_OP_ADD || op == TAC_OP_SUB || op == TAC_OP_MUL ||
           op == TAC_OP_DIV || op == TAC_OP_POW;
}

static TacInst *find_label_inst(TacProgram *p, const char *label) {
    if (!label)
        return NULL;
    for (TacInst *q = p->head; q; q = q->next)
        if (q->op == TAC_OP_LABEL && q->result && strcmp(q->result, label) == 0)
            return q;
    return NULL;
}

static int defined_inside_loop(TacInst *start, TacInst *end_excl, const char *name) {
    if (!name)
        return 0;
    for (TacInst *q = start; q && q != end_excl; q = q->next) {
        if (q->op == TAC_OP_LABEL || q->op == TAC_OP_GOTO)
            continue;
        if (q->op == TAC_OP_IF_LT || q->op == TAC_OP_IF_GT || q->op == TAC_OP_IF_LE ||
            q->op == TAC_OP_IF_GE || q->op == TAC_OP_IF_EQ || q->op == TAC_OP_IF_NE)
            continue;
        if (q->result && strcmp(q->result, name) == 0)
            return 1;
    }
    return 0;
}

static int operand_invariant(TacInst *start, TacInst *end_excl, const char *o) {
    if (!o)
        return 1;
    if (opt_is_num(o))
        return 1;
    return !defined_inside_loop(start, end_excl, o);
}

static int defined_once_in_loop(TacInst *start, TacInst *end_excl, const char *name) {
    int n = 0;
    for (TacInst *q = start; q && q != end_excl; q = q->next) {
        if (q->op == TAC_OP_LABEL || q->op == TAC_OP_GOTO)
            continue;
        if (q->result && name && strcmp(q->result, name) == 0)
            n++;
    }
    return n == 1;
}

static void splice_before(TacProgram *p, TacInst *target, TacInst *node) {
    /* Detach `node` from its position, then insert it right before `target`. */
    TacInst *pn = node->prev;
    TacInst *nn = node->next;
    if (pn)
        pn->next = nn;
    else
        p->head = nn;
    if (nn)
        nn->prev = pn;
    else
        p->tail = pn;

    TacInst *tp = target->prev;
    node->prev = tp;
    node->next = target;
    if (tp)
        tp->next = node;
    else
        p->head = node;
    target->prev = node;
}

static int licm_one_loop(TacProgram *p, TacInst *header, TacInst *back_goto) {
    int hoisted = 0;
    int safety = 0;
    int progress = 1;
    while (progress && safety++ < 64) {
        progress = 0;
        TacInst *cur = header->next;
        while (cur && cur != back_goto) {
            TacInst *next = cur->next;
            int hoist_this = 0;
            const char *opstr = "?";
            /* Case 1: binary op with both operands invariant. */
            if (is_binop(cur->op) && cur->result && cur->result[0] == 't' &&
                operand_invariant(header, back_goto, cur->arg1) &&
                operand_invariant(header, back_goto, cur->arg2) &&
                defined_once_in_loop(header, back_goto, cur->result)) {
                hoist_this = 1;
                opstr = cur->op == TAC_OP_ADD   ? "+"
                        : cur->op == TAC_OP_SUB ? "-"
                        : cur->op == TAC_OP_MUL ? "*"
                        : cur->op == TAC_OP_DIV ? "/"
                                                : "^";
            }
            /* Case 2: invariant copy. */
            else if (cur->op == TAC_OP_COPY && cur->result && cur->result[0] == 't' &&
                     operand_invariant(header, back_goto, cur->arg1) &&
                     defined_once_in_loop(header, back_goto, cur->result)) {
                hoist_this = 1;
                opstr = "=";
            }
            if (hoist_this) {
                if (cur->op == TAC_OP_COPY)
                    printf("    LICM: hoist `%s = %s` out of loop starting at %s\n",
                           cur->result, cur->arg1 ? cur->arg1 : "",
                           header->result ? header->result : "?");
                else
                    printf("    LICM: hoist `%s = %s %s %s` out of loop starting at %s\n",
                           cur->result, cur->arg1 ? cur->arg1 : "", opstr,
                           cur->arg2 ? cur->arg2 : "",
                           header->result ? header->result : "?");
                splice_before(p, header, cur);
                hoisted++;
                progress = 1;
            }
            cur = next;
        }
    }
    return hoisted;
}

void licm_optimize(TacProgram *p) {
    if (!p || !p->head)
        return;
    int total = 0;
    /* Find every back-edge: goto X where X is a label that occurs
       earlier in the list. */
    for (TacInst *q = p->head; q; q = q->next) {
        if (q->op != TAC_OP_GOTO || !q->result)
            continue;
        TacInst *target = find_label_inst(p, q->result);
        if (!target)
            continue;
        if (target->id >= q->id)
            continue;
        total += licm_one_loop(p, target, q);
    }
    if (total == 0)
        printf("    LICM: no loop-invariant computations detected.\n");
    else
        printf("    LICM: %d instruction(s) hoisted out of loops.\n", total);
}
