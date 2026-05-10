# Module 7 — Code Optimisation

`optimize.c` runs all the project-required transformations on the TAC
emitted by Module 6, printing before/after listings for each pass.

## Passes

| # | Pass | Implementation summary |
|---|------|------------------------|
| 1 | **Constant propagation**   | Intra-basic-block; stops at labels / jumps to remain sound. |
| 2 | **Constant folding**       | Evaluates `arg1 op arg2` for `+ - * /` on numeric literals. |
| 3 | **Common subexpression elimination (CSE)** | Reuses identical `+`/`*` results within the same basic block. |
| 4 | **Unreachable code removal** (Task 4) | Drops instructions after an unconditional `goto` until the next label. |
| 5 | **Loop-invariant code motion (LICM)** (Task 5) | Detects natural loops by finding back-edges (`goto X` whose label `X` precedes it) and hoists invariant binary computations into the pre-header. Fixed-point iteration handles cascading hoists. |
| 6 | **Dead code elimination**  | Conservative: a temp def is dead only if *no* instruction in the program reads it. |

## Demonstrating LICM
```
make minicc
./minicc --opt samples/cases/06_licm.mini
```
The output for `06_licm.mini` shows `t1 = k * 4` being hoisted *out* of the `while (i < n)` loop into the pre-header (above `L0:`).

## Task 6 — performance comparison
```
chmod +x ../../tools/opt_bench.sh
../../tools/opt_bench.sh samples/cases/06_licm.mini
```
Runs `./minicc` and `./minicc --opt` 5 × 80 times, prints a comparison table with average per-compile times and a **speedup ratio** column.
