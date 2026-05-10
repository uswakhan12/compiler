# Module 4 — FIRST / FOLLOW / LL(1) Parsing Table

Two complementary tools:

| File | Purpose |
|------|---------|
| `ll1.c`         | Computes FIRST, FOLLOW and the LL(1) table for the **fixed** target grammar of §5.1. |
| `ll1_generic.c` | Reads an **arbitrary** grammar from a text file and computes FIRST, FOLLOW and the LL(1) table. Detects and reports LL(1) conflicts. |

## Grammar file format
```
LHS -> rhs1 rhs2 ... | rhs1 ... | epsilon
# lines starting with '#' or blank lines are ignored
```
Use `epsilon` (or `eps`) for empty productions; `$` is the end marker.

## Build
```
make ll1 ll1_generic
```

## Run
```
./ll1                                        # fixed grammar (project spec)
./ll1_generic grammars/expr.txt              # generic — expression grammar
./ll1_generic grammars/stmt.txt              # generic — small statement grammar
```

`grammars/expr.txt` matches §5.1 exactly; `grammars/stmt.txt` exercises an `if/while/assignment` grammar to demonstrate the tool works on **any** input grammar.
