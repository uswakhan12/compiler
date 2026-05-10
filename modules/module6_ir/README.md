# Module 6 — Intermediate Representation (Three-Address Code)

| File | Purpose |
|------|---------|
| `tac.c`     | `TacProgram` / `TacInst` data structures; pretty-printer (`tac_print`). |
| `codegen.c` | Walks the AST and emits TAC for every statement and expression. |

## Supported TAC forms

| Form | Example |
|------|---------|
| Binary op       | `t = a + b` |
| Unary op        | `t = -a` |
| Copy/assign     | `x = y`,  `t = 5` |
| Unconditional jump | `goto L` |
| Conditional jump   | `if a < b goto L` |
| Label              | `L:` |
| `param` / `call`   | `param x`, `t = call f` |
| `return`           | `return x`, internal `_ret` for AST returns |
| Built-ins          | `t = log(x)`, `t = exp(x)` |
| Type cast          | `t = (float) x` |
| **Array load**     | `t = a[i]` |
| **Array store**    | `a[i] = t` |

The array forms are emitted for `int a[10]; a[i] = expr; ... = a[i];` constructs added to the integrated `minicc` grammar (Module 2).

## Build / run
```
make minicc
./minicc samples/cases/05_array.mini     # shows array TAC
```
