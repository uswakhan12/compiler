#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "tac.h"

/* Run every Module-7 pass and print before/after listings. */
TacProgram *optimize_tac(TacProgram *p);

/* Module 7 Task 5: loop-invariant code motion (LICM).
   Detects natural loops (back-edges to earlier labels) and hoists
   loop-invariant binary computations into the loop pre-header. */
void licm_optimize(TacProgram *p);

#endif
