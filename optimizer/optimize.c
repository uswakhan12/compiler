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
    /* Conservative DCE: a temp def is dead only if NO instruction
       anywhere in the program reads it. A flow-sensitive analysis
       would catch more cases but requires reaching-definitions; this
       version is sound under arbitrary control flow. */
    for (TacInst *in = p->head; in; in = in->next) {
        if (!in->result)
            continue;
        if (in->op == TAC_OP_LABEL)
            continue;
        if (in->result[0] != 't')
            continue;
        /* Skip ops that have side effects beyond their result. */
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
            in->op != TAC_OP_IF_LT && in->op != TAC_OP_IF_GT && in->op != TAC_OP_IF_LE &&
            in->op != TAC_OP_IF_GE && in->op != TAC_OP_GOTO)
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
    /* Intra-basic-block constant propagation only: stop at labels, jumps,
       and any subsequent re-definition of the temp.  Crossing a label is
       unsound because a value at the label may be reached from multiple
       definitions on different control-flow paths (e.g. the two
       branches of an `if`). */
    for (TacInst *in = p->head; in; in = in->next) {
        if (in->op != TAC_OP_COPY || !in->result || in->result[0] != 't')
            continue;
        if (!is_num(in->arg1))
            continue;
        const char *val = in->arg1;
        for (TacInst *q = in->next; q; q = q->next) {
            if (q->op == TAC_OP_LABEL || q->op == TAC_OP_GOTO)
                break;
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
    /* Reuse identical binary ops on same operands within the same basic
       block. We only consider candidate matches that lie between the
       most recent label-or-program-start and the current instruction,
       which is sound without a full data-flow analysis. */
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

/* ------------------------------------------------------------------ */
/* Module 7 Task 5 — Loop-Invariant Code Motion (LICM)                 */
/* ------------------------------------------------------------------ */

/* TAC for `while` always emits:
       Lstart:
         ... cond ...
         if cond == 0 goto Lend
         ... body ...
         goto Lstart
       Lend:
   We detect natural loops by looking for `goto LABEL` whose target
   appears earlier in the instruction list (back-edge). The instructions
   in [target_label_inst, goto_inst] form a single-block loop body. */

static int is_binop(TacOp op) {
    return op == TAC_OP_ADD || op == TAC_OP_SUB || op == TAC_OP_MUL || op == TAC_OP_DIV ||
           op == TAC_OP_POW;
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
    if (is_num(o))
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
    /* Pre-header = a new label inserted *before* `header`, into which we
       hoist invariant computations. For simplicity we hoist directly
       before the header label — semantically equivalent if there is no
       other goto into header from above, which is the case for our
       compiler since `while` only generates one such label. */
    int hoisted = 0;
    int safety = 0;
    /* Repeat until fixed-point: hoisting may make further insts invariant. */
    int progress = 1;
    while (progress && safety++ < 64) {
        progress = 0;
        TacInst *cur = header->next;
        while (cur && cur != back_goto) {
            TacInst *next = cur->next;
            if (is_binop(cur->op) && cur->result && cur->result[0] == 't' &&
                operand_invariant(header, back_goto, cur->arg1) &&
                operand_invariant(header, back_goto, cur->arg2) &&
                defined_once_in_loop(header, back_goto, cur->result)) {
                printf("    LICM: hoist `%s = %s %s %s` out of loop starting at %s\n",
                       cur->result, cur->arg1 ? cur->arg1 : "",
                       cur->op == TAC_OP_ADD   ? "+"
                       : cur->op == TAC_OP_SUB ? "-"
                       : cur->op == TAC_OP_MUL ? "*"
                       : cur->op == TAC_OP_DIV ? "/"
                                               : "^",
                       cur->arg2 ? cur->arg2 : "", header->result ? header->result : "?");
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
        /* back-edge only if target precedes q */
        int target_id = target->id;
        if (target_id >= q->id)
            continue;
        total += licm_one_loop(p, target, q);
    }
    if (total == 0)
        printf("    LICM: no loop-invariant computations detected.\n");
    else
        printf("    LICM: %d instruction(s) hoisted out of loops.\n", total);
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
    printf("\n=== Module 7 — Task 5: Loop-invariant code motion (LICM) ===\n");
    printf("(Hoist computations whose operands do not change inside the loop.)\n");
    licm_optimize(p);
    tac_print(p);
    printf("\n=== Module 7 — Technique 5: Dead code elimination ===\n");
    printf("(Remove unused temp definitions that do not affect control flow.)\n");
    pass_dead_code_elimination(p);
    prune_dead(p);
    tac_print(p);
    return p;
}
