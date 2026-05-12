# CS-346 Mini-Compiler

A course-scale compiler for a small C-like language (`.mini` sources). The project is organized as **eight modules** (lexical analysis through LLVM IR emission), each mapped to directories and binaries. The integrated driver is **`minicc`**, which chains parsing, semantic analysis, three-address code (TAC) generation, optional optimisation passes, and optional LLVM IR lowering.

This document describes **repository layout**, **build mechanics**, **the source language**, **every major subsystem**, **CLI usage**, and **how the teaching/demo binaries relate to the full pipeline**.

---

## Table of contents

1. [Prerequisites](#prerequisites)
2. [Build system](#build-system)
3. [Repository layout](#repository-layout)
4. [The `.mini` source language](#the-mini-source-language)
5. [End-to-end pipeline (`minicc`)](#end-to-end-pipeline-minicc)
6. [Module 1 — Lexical analysis](#module-1--lexical-analysis)
7. [Module 2 — Parsing (postfix, prefix, infix, parse tree)](#module-2--parsing-postfix-prefix-infix-parse-tree)
8. [Module 3 — Extended expression grammar](#module-3--extended-expression-grammar)
9. [Module 4 — FIRST / FOLLOW / LL(1)](#module-4--first--follow--ll1)
10. [Module 5 — AST, symbol table, semantics](#module-5--ast-symbol-table-semantics)
11. [Module 6 — TAC and code generation](#module-6--tac-and-code-generation)
12. [Module 7 — Optimisations](#module-7--optimisations)
13. [Module 8 — LLVM IR backend](#module-8--llvm-ir-backend)
14. [Scripts and samples](#scripts-and-samples)
15. [Troubleshooting](#troubleshooting)

---

## Prerequisites

| Tool | Role |
|------|------|
| **flex** | Generates C scanners from `.l` files. |
| **bison ≥ 3.0** | Generates C parsers from `.y` files. On macOS, Apple’s `/usr/bin/bison` is often 2.3; the `Makefile` prefers Homebrew’s bison when present (`/opt/homebrew/opt/bison/bin/bison`). |
| **gcc or clang** | C11 compilation (`-std=c11 -Wall -Wextra -O2`). |
| **bc** (optional) | Used by `tools/opt_bench.sh` for averaging runtimes. |
| **clang** (optional) | Compiles emitted `.ll` to executables; `llvm/generate_ir.sh` uses it for reference IR from bundled C tests. |

**macOS quick install (typical):**

```sh
brew install flex bison
```

Linux commonly has a new enough bison in package managers; link flags use `-lfl` for flex (see `Makefile`).

---

## Build system

The **`Makefile`** is the single source of truth for targets and linking.

### Platform-specific behaviour

- **`UNAME_S == Darwin`**: `LDLIBS = -ll -lm` (Apple flex uses `libl` not `libfl`).
- **Else**: `LDLIBS = -lfl -lm`.
- **Bison on macOS**: if `$(HBREW_BIN)/bison` exists, `BISON` is set to that path; otherwise `bison` on `PATH`.

### Include paths and generated code

- **`CPPFLAGS = -Iinclude -Ibuild`**: shared headers live under `include/`; bison emits token and `YYSTYPE` definitions into `build/*.tab.h`, which the flex scanner for `minicc` includes as `minicc.tab.h`.
- **`BUILD = build`**: all generated flex/bison C sources are written here (e.g. `build/minicc.tab.c`, `build/lex.minicc.c`). The directory is created by the `dirs` target.

### Primary targets

| Target | Output | Purpose |
|--------|--------|---------|
| `all` (default) | `minicc`, `calc` (= postfix+prefix+infix), `extended`, `ll1`, `ll1_generic`, `lab05`, `postfix_eval`, `parse_tree` | Full teaching set + integrated compiler. |
| `minicc` | `./minicc` | Full pipeline binary (lexer+parser linked with semantic, TAC, optimiser, LLVM emitter, `main.c`). |
| `lab05` | `./lab05` | Standalone flex lexer from `lexer/lab05_lexer.l`. |
| `postfix_eval` | `./postfix_eval` | Standalone postfix RPN evaluator (`lexer/postfix_eval.c`). |
| `postfix`, `prefix`, `infix` | `./postfix`, etc. | Pairwise flex+bison calculators. |
| `extended` | `./extended` | Extended arithmetic grammar with bison **`-t`** (trace tables available with `--debug`). |
| `parse_tree` | `./parse_tree` | Parse/syntax tree demo with traversals. |
| `ll1`, `ll1_generic` | `./ll1`, `./ll1_generic` | Fixed and file-driven LL(1) table builders. |
| `clean` | — | Removes `build/` and all listed binaries. |
| `run-sample` | — | Runs `./minicc samples/demo.mini`. |
| `run-all` | — | Runs `tools/run_all_modules.sh` after `all`. |

**Build command:**

```sh
make
```

---

## Repository layout

| Path | Contents |
|------|----------|
| `lexer/` | `lexer.l` (minicc tokens), `lab05_lexer.l`, `postfix_eval.c`. |
| `parser/` | `minicc.y` (main grammar), `postfix.*`, `prefix.*`, `infix.*`, `parse_tree.*`, `extended.*`. |
| `first_follow/` | `ll1.c`, `ll1_generic.c`, `grammars/*.txt`. |
| `semantic/` | `ast.c`, `symtab.c`, `semantics.c`. |
| `ir/` | `tac.c` (IR data structure + printer), `codegen.c` (AST → TAC). |
| `optimizer/` | One `.c` per optimisation pass + `optimize.c` orchestrator + `opt_util.c`. |
| `llvm/` | `llvm_emit.c`, `generate_ir.sh`, `test1.c` / `test2.c`, `out/` (generated `.ll`). |
| `include/` | Public headers: `ast.h`, `symtab.h`, `semantics.h`, `tac.h`, `codegen.h`, `optimize.h`, `llvm_emit.h`, `token_dump.h`; internal `opt_passes.h` for optimiser units. |
| `src/` | `main.c` (CLI driver), `token_dump.c` (token stream printer). |
| `samples/` | `.mini` programs; `cases/` holds numbered regression-style examples. |
| `tools/` | `run_all_modules.sh`, `opt_bench.sh`. |
| `build/` | **Generated** — do not edit by hand; recreated by `make`. |

---

## The `.mini` source language

The grammar implemented in `parser/minicc.y` describes a **statement list** (implicit outer program). There are no user-defined functions in the surface syntax; the only “calls” are builtins **`log`** and **`exp`**.

### Lexical rules (`lexer/lexer.l`)

- **Whitespace**: spaces, tabs, newlines ignored.
- **Comments**: `//` to end of line; nested `/* ... */` via a flex start condition `COMMENT` and a `comment_depth` counter.
- **Keywords**: `int`, `float`, `if`, `else`, `while`, `return`, `log`, `exp`.
- **Multi-char operators**: `==`, `!=`, `<=`, `>=`; single `=`, comparisons `<` `>`, arithmetic `+ - * / ^`, punctuation `; , { } [ ] ( )`.
- **Identifiers**: `[A-Za-z_][A-Za-z0-9_]*`, semantic value `yylval.str` (heap-allocated via `strdup`).
- **Literals**: integer (`atoll`), float (must contain a dot: pattern `[0-9]+"."[0-9]+`, `strtod`).
- **Errors**: unknown character → message on stderr and token `0` (end of input), which aborts the parse.

### Syntax and precedence (`parser/minicc.y`)

Non-terminals include `program`, `stmt_list`, `stmt`, `expr`, `block`, `decl_stmt`, `assign_stmt`, `optional_else`.

**Statements**

- Declarations: `int x;`, `float y;`, `int a[10];`, `float b[20];`.
- Assignment: `x = expr;`, `a[i] = expr;`.
- Control: `if (expr) stmt [else stmt]`, `while (expr) stmt`, `{ stmt_list }`.
- Expression statement: `expr;`.
- Return: `return expr;` (lowers to `TAC_OP_RETURN` for spec alignment).

**Expression grammar (excerpt)**

- Literals, variables, indexed load `id[expr]`.
- Binary operators with precedence (low → high): `== !=` `< >` `<= >=`, then `+ -`, then `* /`, then right-associative `^`.
- Unary `-` with `%prec UNARY`.
- Parenthesised expressions.
- Calls: `log(expr)`, `exp(expr)`.

**Parser note**: `%expect 1` documents one expected shift/reduce ambiguity (classic **dangling else**): `else` binds to the nearest `if` in LALR(1) terms.

### Semantic model (`semantic/semantics.c`)

- **Types**: `int`, `float`. Mixed-type arithmetic on binary ops promotes to `float` when either operand is `float` (recorded in `AstNode.inferred_type`).
- **Comparisons** produce an **integer** 0/1-style boolean in the AST typing (`inferred_type == TYPE_INT` for comparison nodes).
- **Arrays**: index expression must be `int`; storing a `float` into an `int` array (or assigning `float` to `int` variable) is a **semantic error**.
- **Scope**: `AST_BLOCK` pushes/pops a symbol table scope. Redeclaration in the **same** scope is illegal; redeclaration in an inner scope **shadows** outer bindings (see symbol table).

---

## End-to-end pipeline (`minicc`)

`src/main.c` is the driver. Rough order of phases:

1. **Parse** (`yyparse`) — flex/bison read `yyin` and set global `AstNode *g_parse_result`.
2. **`semantic_check(g_parse_result)`** — builds/uses symbol table, annotates types, may rewrite names to mangled form.
3. **`generate_tac`** — AST → `TacProgram`.
4. **Print** “before optimisation” TAC.
5. **Optional** `optimize_tac` — full Module 7 pass pipeline.
6. **Optional** `emit_llvm_ir` — TAC (+ AST for decl types) → textual LLVM IR.

### Command-line interface

```
minicc [options] [file.mini]
```

| Option | Behaviour |
|--------|------------|
| `-h`, `--help` | Usage summary. |
| *(default)* | Parse → semantics → TAC print (no optimiser, no LLVM). |
| `--tokens` | Lexer only: human-readable token stream via `print_token_stream`. Reads `file` if given, else stdin. **Exits before parse.** |
| `--check` | Semantic pass only; prints `semantic check passed.` on success. |
| `--tac` | Explicit no-op flag (default path already prints TAC). |
| `--opt` | Runs `optimize_tac` after initial TAC; prints each pass listing (see Module 7). |
| `--emit-llvm[=F]` | After TAC (and after `--opt` if set), writes LLVM IR to file `F` or stdout. |
| `--pipeline` | Requires a file. Forces: token dump → parse → semantics → TAC → **full opt** → **LLVM to stdout**. Prints phased banners. |

**Typical LLVM workflow:**

```sh
./minicc --emit-llvm=demo.ll samples/cases/01_arith.mini
clang -Wno-override-module demo.ll -o demo_exe && ./demo_exe
```

---

## Module 1 — Lexical analysis

### Integrated lexer (`lexer/lexer.l` + `build/lex.minicc.c`)

- Compiled as part of **`minicc`** with `parser/minicc.y` so token codes match `yylval` union fields (`AstNode *`, `char *`, `long long`, `double`).
- **`%option noyywrap yylineno`**: single-file input without libfl wrap requirement; line numbers for errors.

### Lab 05 lexer (`lab05` binary)

- Source: `lexer/lab05_lexer.l` — a separate flex specification for an older “procedure / function” style demo used by `run_all_modules.sh`.
- Built as `$(BUILD)/lab05.c` then linked to `lab05`.

### Token stream debug (`src/token_dump.c`)

- **`print_token_stream(FILE *in)`**: `yyrestart(in)` then loops `yylex()` until 0.
- Prints token names and literal values (identifiers and numbers). Used by `--tokens` and `--pipeline` phase 1.

### Postfix evaluator (`postfix_eval`)

- **`lexer/postfix_eval.c`**: a small standalone program reading RPN integers/operators from stdin (demonstrated in `run_all_modules.sh` with `4 8 + 3 *`).

---

## Module 2 — Parsing (postfix, prefix, infix, parse tree)

These targets are **pedagogical parsers**, not used inside `minicc`.

| Binary | Flex | Bison | Role |
|--------|------|-------|------|
| `postfix` | `parser/postfix.l` | `parser/postfix.y` | RPN calculator. |
| `prefix` | `parser/prefix.l` | `parser/prefix.y` | Polish notation calculator. |
| `infix` | `parser/infix.l` | `parser/infix.y` | Infix with parentheses. |
| `parse_tree` | `parser/parse_tree.l` | `parser/parse_tree.y` | Builds explicit tree; prints traversals. |

Each pair is compiled with `-I$(BUILD)` and only the generated lexer/parser objects — no AST/TAC linkage.

---

## Module 3 — Extended expression grammar

**`extended`** (`parser/extended.l` + `parser/extended.y`):

- Bison invoked with **`-t`** in the `Makefile` to enable trace facilities; run with **`--debug`** for bison debug output (as in `run_all_modules.sh`).
- Extends arithmetic with features such as **`^`**, **`log`**, **`exp`** at the **expression-grammar** level (separate from the full `minicc` program grammar, which already includes these).

---

## Module 4 — FIRST / FOLLOW / LL(1)

### Fixed grammar (`first_follow/ll1.c` → `ll1`)

Implements the classic expression grammar:

- `E  -> T E'`
- `E' -> + T E' | - T E' | ε`
- `T  -> F T'`
- `T' -> * F T' | / F T' | ε`
- `F  -> ( E ) | id | num`

Computes FIRST/FOLLOW sets and prints an LL(1) parsing table (terminal × non-terminal).

### Grammar from file (`first_follow/ll1_generic.c` → `ll1_generic`)

- **Input format**: lines `LHS -> alt1 | alt2 | ...` where alternatives are space-separated symbols; `#` comments and blank lines ignored.
- **Epsilon**: token `epsilon`, `eps`, or `ε`.
- **End of input**: `$` added to FOLLOW of start symbol.
- **Symbol classification**: any symbol appearing on a **left-hand side** is a non-terminal; all other tokens are terminals.

Bundled examples: `first_follow/grammars/expr.txt`, `first_follow/grammars/stmt.txt`.

---

## Module 5 — AST, symbol table, semantics

### AST (`include/ast.h`, `semantic/ast.c`)

- **Discriminated union** via `AstKind` and `struct AstNode` with a `union` of payloads (`decl`, `assign`, `iff`, `binop`, etc.).
- **Linked lists**: `AstNode.next` chains statements in program/block lists; `ast_stmt_append` builds lists; `ast_program` wraps the list as `AST_PROGRAM` (reusing the `block` union field `first` for the statement list — the semantic walker treats `AST_PROGRAM` like a block for iteration).
- **Line numbers**: propagated for error reporting.
- **`ast_free`**: frees the whole tree (allocated names freed appropriately in implementation).

### Symbol table (`include/symtab.h`, `semantic/symtab.c`)

- **Stack of `Scope`**: each scope holds a linked list of `Symbol` entries (`name`, `mangled`, `type`).
- **`symtab_insert`**: fails (returns NULL) if the same **source** `name` already exists in the **current** scope.
- **Shadowing**: if the name exists in an **enclosing** scope, the new symbol still inserts, but **`mangled`** becomes `name.N` with a monotonically increasing `N` (`g_shadow_counter`). Code generation and LLVM use these distinct strings for separate storage slots.
- **`symtab_reset`**: pops all scopes and resets the shadow counter.

### Semantic checker (`semantic/semantics.c`)

- **`semantic_check`**: resets symtab, pushes global scope, walks `AST_PROGRAM` statement list.
- **`check_expr`**: recursively validates expressions, sets `inferred_type`, checks array rules and builtins.
- **`rewrite_name`**: after successful `symtab_lookup`, if the symbol’s `mangled` differs from the AST’s stored identifier, the AST string is **replaced** with a `strdup` of the mangled name so later phases emit correct TAC/LLVM symbols.

**Return value**: count of semantic errors (non-zero → `main` exits after `ast_free`).

---

## Module 6 — TAC and code generation

### TAC IR (`include/tac.h`, `ir/tac.c`)

- **`TacProgram`**: intrusive doubly-linked list (`head`/`tail`), counters `next_label`, `next_temp`.
- **`TacInst`**: `op`, string slots `result`, `arg1`, `arg2`, `extra`, `id`, `dead` flag (used by optimiser).
- **Factory helpers**: `tac_new`, `tac_emit`, `tac_new_temp` (`t0`, `t1`, …), `tac_new_label` (`L0`, …), `tac_free_program`, `tac_print`.

**Representative opcodes**

| Op | Meaning (conceptual) |
|----|----------------------|
| `TAC_OP_ADD` … `TAC_OP_POW` | Binary arithmetic. |
| `TAC_OP_NEG` | Unary negation. |
| `TAC_OP_ASSIGN` | Store to scalar variable: `result = arg1` form in printer. |
| `TAC_OP_COPY` | Copy between temporaries or constants (used in compare lowering). |
| `TAC_OP_GOTO`, `TAC_OP_LABEL` | Unconditional control flow. |
| `TAC_OP_IF_*` | Conditional branches (compare `arg1` and `arg2`, target `result`). |
| `TAC_OP_PARAM` / `TAC_OP_CALL` | Abstract call sequence (`param` then `t = call f, arity`). |
| `TAC_OP_RETURN` | `return arg1`. |
| `TAC_OP_ARR_LOAD` / `TAC_OP_ARR_STORE` | Indexed load/store (`result`, `arg1`, `arg2` roles as in `tac_print`). |
| `TAC_OP_CAST_INT_TO_FLOAT` | Explicit widen for mixed-type arithmetic. |

### Code generator (`ir/codegen.c`)

- **`generate_tac(AstNode *prog)`**: allocates `TacProgram`, walks statements.
- **`gen_expr`**: returns a **newly allocated** string holding either a literal, variable name, or temp name. Frees subexpressions’ temps after emitting consuming instructions.
- **Comparisons**: lowered to jumps and copies of `0`/`1` into a fresh temp (`gen_compare`) — no single “icmp” opcode at TAC level.
- **Mixed float/int**: uses `promote_if_needed` to insert `TAC_OP_CAST_INT_TO_FLOAT` when the AST’s inferred result type is `float`.
- **Pow**: direct `TAC_OP_POW` (lowered in LLVM backend appropriately).
- **Builtin math**: emits `param`, then `call` with function name `log` or `exp` and arity `"1"` (string) to match the calling convention expected by the LLVM layer.
- **Control structures**:
  - **`if`**: evaluate cond; if cond `== 0` jump to else label; then-branch; `goto` merge; else label; optional else; merge label.
  - **`while`**: top label; evaluate cond; exit on false; body; `goto` top; end label.

Declarations do not emit TAC by themselves (stack slots are implied in LLVM from AST; TAC uses names directly).

---

## Module 7 — Optimisations

**Orchestrator**: `optimizer/optimize.c` → **`optimize_tac`** (`include/optimize.h`).

Passes run in a deliberate order (comments in `optimize.c`): constant information flows into folding and algebraic simplification; CSE consumes stable patterns; unreachable code removal runs before LICM so dead loops are not analysed; **dead code elimination** runs last.

| File | Pass |
|------|------|
| `optimizer/constant_propagation.c` | Replace uses of temps known to hold constants. |
| `optimizer/constant_folding.c` | Evaluate ops with constant operands. |
| `optimizer/algebraic.c` | Identities (`x+0`, `x*1`, `x*0`, `x/1`, …). |
| `optimizer/cse.c` | Common subexpression elimination. |
| `optimizer/unreachable.c` | Remove code after unconditional `goto` / `return`. |
| `optimizer/licm.c` | Loop-invariant code motion (natural loops via back-edges); **`licm_optimize`** also declared in `optimize.h` for direct experiments. |
| `optimizer/dead_code.c` | Remove unused temp definitions without control-flow side effects. |
| `optimizer/opt_util.c` | `opt_is_num`, `opt_num_val`, `opt_prune_dead` (`dead` flag sweeping). |

**Internal API**: `include/opt_passes.h` declares pass entry points shared across units.

Each pass is followed by **`tac_print`** so coursework logs show intermediate programs.

---

## Module 8 — LLVM IR backend

**`llvm/llvm_emit.c`** implements **`emit_llvm_ir(FILE *out, AstNode *program, TacProgram *tac)`**.

### Design (summary from source comments)

- Walks the **AST** with `collect_var_types` to record each declared variable’s type and array size.
- Emits a **`define i32 @main()`** (return value of `main` is `i32` in the generated module) containing:
  - `alloca` slots for **every source variable** and **every temp** (`tN`) for a simple non-SSA mapping.
  - `load`/`store` around operations to match TAC’s abstract operands.
- **Integer vs float**: variables use `i32` or `double`; **temp types** inferred by a fixed-point **`infer_temp_types`** pass over TAC (propagates through arithmetic, calls to `log`/`exp`, casts, array element types, etc.).
- **Conditionals**: TAC conditional ops become LLVM `icmp` / `fcmp` + `br` as appropriate.
- **Math builtins**: declares `@log`, `@exp` when used; lowers `param`/`call` sequence.
- **Arrays**: sized `alloca` arrays; GEP + load/store for `TAC_OP_ARR_LOAD` / `TAC_OP_ARR_STORE`.
- **Return**: if the program has explicit `return`, emits `ret`; otherwise ensures a path returns (tracked via `g_has_return`).

The **`samples/`** + **`llvm/out/`** workflow matches coursework demos: emit `.ll`, then `clang` to native code.

**Reference script**: `llvm/generate_ir.sh` compiles `llvm/test1.c` and `llvm/test2.c` to unoptimised and `-O3` IR for comparison with student-generated IR.

---

## Scripts and samples

### `tools/run_all_modules.sh`

- Verifies all teaching binaries exist (`make` required).
- Collects test inputs (`samples/cases/*.mini` if present, else non-`bad_*` samples).
- Runs **Module 1–8** demonstrations in order: token dumps, calculators, extended parser, LL(1) tools, semantic checks (including expected failures on `bad_type.mini` / `bad_scope.mini`), raw TAC, full optimisation listing, optional `opt_bench.sh`, LLVM emission to `llvm/out/`, optional Clang reference, and finally **`minicc --pipeline`** on the first case.

**Usage:**

```sh
chmod +x tools/run_all_modules.sh
./tools/run_all_modules.sh | tee log.txt
```

### `tools/opt_bench.sh`

- Builds two LLVM files from the same source: without `--opt` and with `--opt`.
- Compiles both with **`clang -O0`** deliberately so the backend optimiser does not erase the effect of minicc’s passes.
- Runs each binary **5** times, uses **`bc`** for averages, prints a markdown-style table and speedup ratio.

### Samples

- **`samples/cases/`**: numbered scenarios (`01_arith`, `02_if`, `03_loop`, `04_float`, `05_array`, `06_licm`, `07_func_call`, etc.) exercising features and optimiser behaviour.
- **`samples/bad_type.mini`**, **`samples/bad_scope.mini`**: negative tests for semantic errors.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Bison errors or `%code` failures | System bison too old; install bison 3+ and ensure `Makefile` picks Homebrew on Apple Silicon. |
| Link error `yywrap` | Wrong platform `LDLIBS`; `lexer.l` uses `%option noyywrap` for `minicc`. |
| `parse failed` | Syntax error in `.mini`; stderr shows `yyerror` line. |
| `semantic error` / nonzero exit | Type or scope violation; stderr lists line and message. |
| Empty or odd LLVM | Check that `--emit-llvm` path parsing matches usage (`--emit-llvm=file.ll`); pipeline forces LLVM to stdout. |
| `opt_bench.sh` fails | Needs `bc` and `clang` on PATH and an existing sample path. |

---

## Academic / maintenance notes

- **Generated sources** under `build/` are reproducible from flex/bison; they may appear in the tree after a local build — treat as artifacts.
- **Global parse result** `g_parse_result` and flex/bison globals (`yyin`, `yylineno`) keep the driver small for a course codebase; a production compiler would encapsulate state in a context struct.
- **Security**: this is a teaching compiler; it does not harden against malicious huge inputs or untrusted code execution — only run trusted `.mini` sources.

---

## Licence and course context

This repository is structured for **CS-346 Compiler Construction** (mini-compiler milestones). Refer to your course materials for submission requirements and academic integrity rules.
