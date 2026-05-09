# CS-346 — Compiler construction (mini-compiler)

## Prerequisites

- **flex**, **bison**, **gcc**
- **clang** (optional — Module 8 LLVM IR scripts)
- **bc** (optional — optimisation timing in `tools/opt_bench.sh`)

On macOS the Makefile links **`-ll`** for Flex; on Linux it uses **`-lfl`**.

## Build

```bash
make
```

Produces `minicc`, calculator binaries (`postfix`, `prefix`, `infix`), `extended`, `ll1`.

## Test cases

Compiler inputs live under **`samples/cases/*.mini`**. Add or remove `.mini` files here — anything in this folder is used automatically (sorted order).

Defaults included:

| File | Idea |
|------|------|
| `01_arith.mini` | Arithmetic |
| `02_if.mini` | `if` / `else` |
| `03_loop.mini` | `while` |
| `04_float.mini` | `float` / promotion |

If **`samples/cases/`** is empty, the runner falls back to **`samples/*.mini`** (excluding `bad_*.mini`).

Failure demos for semantic errors: `samples/bad_type.mini`, `samples/bad_scope.mini`.

## Run everything (all modules to terminal)

```bash
chmod +x tools/run_all_modules.sh
./tools/run_all_modules.sh
```

Save output:

```bash
./tools/run_all_modules.sh | tee log.txt
```

Shortcut:

```bash
make run-all
```

## `minicc` usage

| Command | Meaning |
|---------|---------|
| `./minicc FILE.mini` | Parse → semantics → TAC |
| `./minicc --tokens FILE.mini` | Token stream only |
| `./minicc --check FILE.mini` | Semantic check only |
| `./minicc --opt FILE.mini` | TAC + optimisation passes |
| `./minicc --pipeline FILE.mini` | Tokens through optimised TAC (single file) |

Example:

```bash
./minicc samples/cases/01_arith.mini
./minicc --pipeline samples/cases/01_arith.mini
```

## Other targets

- Calculators: `./postfix`, `./prefix`, `./infix`
- Extended grammar: `./extended < samples/extended_expr.txt`
- FIRST/FOLLOW tool: `./ll1`
- LLVM sample IR: `chmod +x llvm/generate_ir.sh && ./llvm/generate_ir.sh`

See **`Makefile`** for exact rules.
