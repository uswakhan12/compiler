/*
 * Module 7 — Optimisation orchestrator.
 *
 * This file used to contain every optimisation pass in one monolith.
 * After the Module 7 Task 1 ("Refactoring") split each technique now
 * lives in its own translation unit:
 *
 *   opt_util.c              shared helpers (is_num, prune_dead)
 *   constant_propagation.c  Technique 1
 *   constant_folding.c      Technique 2
 *   algebraic.c             Technique 2b (identity rewrites)
 *   cse.c                   Technique 3
 *   unreachable.c           Task 4 (after goto / return)
 *   licm.c                  Task 5 (loop-invariant code motion)
 *   dead_code.c             Technique 5 (whole-program DCE)
 *
 * The role of this file is to (1) print the labelled "before / after"
 * banner that the rubric calls for and (2) run each pass in an order
 * that lets later passes consume the simplifications produced by
 * earlier ones (e.g. const-prop feeds const-fold feeds algebraic feeds
 * CSE; unreachable runs before LICM so dead loops do not get scanned;
 * DCE runs last to sweep up everything left over).
 */

#include "opt_passes.h"
#include "optimize.h"
#include "tac.h"
#include <stdio.h>

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

    printf("\n=== Module 7 — Technique 2b: Algebraic simplification ===\n");
    printf("(Apply identities such as x+0, x*1, x*0, x/1.)\n");
    pass_algebraic_simplification(p);
    tac_print(p);

    printf("\n=== Module 7 — Technique 3: Common subexpression elimination (CSE) ===\n");
    printf("(Reuse result of identical operations on same operands.)\n");
    pass_cse(p);
    tac_print(p);

    printf("\n=== Module 7 — Task 4: Unreachable code removal ===\n");
    printf("(Delete instructions after unconditional goto or return.)\n");
    pass_unreachable(p);
    tac_print(p);

    printf("\n=== Module 7 — Task 5: Loop-invariant code motion (LICM) ===\n");
    printf("(Hoist computations whose operands do not change inside the loop.)\n");
    licm_optimize(p);
    tac_print(p);

    printf("\n=== Module 7 — Technique 5: Dead code elimination ===\n");
    printf("(Remove unused temp definitions that do not affect control flow.)\n");
    pass_dead_code_elimination(p);
    tac_print(p);

    return p;
}
