# CS-346 — Compiler Construction (Mini-Compiler)

A complete mini-compiler implementing **every** module of the CS-346
specification, from lexical analysis through optimisation to native code
generation via LLVM. The codebase is organised one folder per module.

```
modules/
├── module1_lexer/          Lab 05 baseline lexer + postfix evaluator
│                           + full-project lexer (lexer.l)
├── module2_parser/         Postfix / Prefix / Infix calculators,
│                           parse-tree-with-traversals demo,
│                           minicc.y integrated grammar
├── module3_extended/       Extended grammar (^, log, exp) + YYDEBUG
├── module4_first_follow/   Fixed-grammar LL(1) tool + generic LL(1)
│                           tool that reads grammars from a file
├── module5_semantics/      Type & scope checker, AST, symbol table
├── module6_ir/             TAC IR + code generator (incl. arrays)
├── module7_optimizer/      Const-prop, const-fold, CSE, unreachable-code,
│                           LICM (Task 5), dead-code elimination
└── module8_llvm/           TAC → LLVM IR backend (built into minicc)
                            + Clang reference programs (O0 vs O3)
```

Shared headers live in `include/`; the integrated driver in `src/`;
test inputs in `samples/`; shell helpers in `tools/`.

## Prerequisites

* **flex**, **bison ≥ 3.0** (macOS users: `brew install bison` — the
  Makefile auto-picks `/opt/homebrew/opt/bison/bin/bison` when present
  because Apple's stock `/usr/bin/bison` is 2.3)
* **gcc** / **clang**
* **bc** (optional — used by `tools/opt_bench.sh`)

## Build

```bash
make            # builds everything
make clean      # remove generated artefacts
```

Generates the following binaries at repo root:

| Binary | Modules | Purpose |
|--------|---------|---------|
| `minicc`        | 1, 2, 5, 6, 7, 8 | Integrated compiler driver. |
| `lab05`         | 1 | Lab 05 baseline lexer. |
| `postfix_eval`  | 1 | Lab 05 Task 2 postfix evaluator (stack transitions). |
| `postfix`, `prefix`, `infix` | 2 | Three calculator notations. |
| `parse_tree`    | 2 | Explicit parse tree + syntax tree + pre/in/post traversal. |
| `extended`      | 3 | Extended grammar with `^`, `log`, `exp`, `YYDEBUG`. |
| `ll1`           | 4 | Fixed grammar from §5.1. |
| `ll1_generic`   | 4 | Reads any grammar from a text file. |

## End-to-end run

```bash
chmod +x tools/run_all_modules.sh
./tools/run_all_modules.sh | tee log.txt
```

This drives every module on every test case in `samples/cases/`, the two
failure-demo files (`samples/bad_type.mini`, `samples/bad_scope.mini`),
prints the optimised TAC, runs the Task-6 timing table, emits LLVM IR
for each test case via our backend, and runs Clang's O0/O3 comparison.

Shortcut:

```bash
make run-all
```

## Quick reference

```bash
./minicc samples/cases/01_arith.mini             # tokens → … → TAC
./minicc --tokens   FILE.mini                    # lexer only
./minicc --check    FILE.mini                    # semantic check only
./minicc --opt      FILE.mini                    # all 7 optimisation passes
./minicc --pipeline FILE.mini                    # complete narrated run
./minicc --emit-llvm=out.ll FILE.mini            # full pipeline + LLVM IR
clang -Wno-override-module out.ll -o out_exe && ./out_exe
```

See each `modules/moduleN_*/README.md` for module-specific details
matching the project specification's section structure.

## Sample inputs

| File | Demonstrates |
|------|--------------|
| `samples/cases/01_arith.mini` | Plain arithmetic, constant folding effect. |
| `samples/cases/02_if.mini`    | `if` / `else` lowering, branch elimination. |
| `samples/cases/03_loop.mini`  | `while` loop lowering. |
| `samples/cases/04_float.mini` | `int → float` promotion. |
| `samples/cases/05_array.mini` | Array declaration, indexed store / load. |
| `samples/cases/06_licm.mini`  | Loop-invariant code motion (LICM hoist). |
| `samples/bad_type.mini`       | Type-mismatch error (semantic). |
| `samples/bad_scope.mini`      | Use-before-declaration (semantic). |
