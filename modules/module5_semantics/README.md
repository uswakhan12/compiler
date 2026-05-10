# Module 5 — Semantic Analysis (Type & Scope Checking)

| File | Purpose |
|------|---------|
| `ast.c`       | AST node constructors + destructor (free). |
| `symtab.c`    | Stack-based symbol table; one `Scope` per `{ … }` block, parent pointer walks find outer-scope names. |
| `semantics.c` | Two passes folded into one walk: **type checking** (mixed int/float → promotion; error on incompatible assignment) and **scope checking** (use-before-decl; redeclaration in same scope; local shadows outer). |

## Behaviour

* Local declarations shadow outer ones (`symtab_lookup_local` vs `symtab_lookup`).
* Implicit `int → float` promotion inside `+ - * /`.
* Explicit error on `int = float` assignment.
* Array index must be integer; array element type propagates through use.

## Build / run
```
make minicc
./minicc --check samples/cases/01_arith.mini       # passes
./minicc samples/bad_type.mini                     # type-mismatch
./minicc samples/bad_scope.mini                    # use before declaration
```
