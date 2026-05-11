#ifndef OPT_PASSES_H
#define OPT_PASSES_H

/* Internal header shared across every optimizer translation unit.
   The public API for the rest of the compiler lives in optimize.h.

   Splitting the optimiser into one file per pass (Module 7 Task 1
   "Refactoring") makes each technique independently testable and lets
   us add a new pass by adding a new .c file plus one orchestrator call. */

#include "tac.h"
#include <stdbool.h>

/* ---- shared helpers (opt_util.c) ----------------------------------- */

/* Returns 1 iff `s` is a complete decimal/integer numeric literal. */
int opt_is_num(const char *s);

/* strtod-style numeric value extraction (only valid if opt_is_num). */
double opt_num_val(const char *s);

/* Remove every TacInst whose `dead` flag is set, freeing it. */
void opt_prune_dead(TacProgram *p);

/* ---- individual passes (one per .c file) --------------------------- */

void pass_constant_propagation(TacProgram *p);
void pass_constant_folding(TacProgram *p);
void pass_algebraic_simplification(TacProgram *p);
void pass_cse(TacProgram *p);
void pass_unreachable(TacProgram *p);
void pass_dead_code_elimination(TacProgram *p);

#endif
