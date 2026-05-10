# Module 1 — Lexical Analysis

| File | Purpose |
|------|---------|
| `lab05_lexer.l`   | Lab 05 Task 1 **baseline** lexer (keywords: `if then begin end procedure function`; identifiers must start with a lowercase letter). |
| `postfix_eval.c`  | Lab 05 Task 2 standalone **postfix expression evaluator** with explicit push/pop stack-transition output. |
| `lexer.l`         | Full-project lexer used by the integrated `minicc` (extended token set: `int float if else while return`, `==`, `!=`, `<=`, `>=`, `^`, `[`, `]`, …). |

## Build
```
make lab05 postfix_eval minicc
```

## Run
```
echo 'procedure foo
if x then begin y = 42 end' | ./lab05

echo "4 8 + 3 *" | ./postfix_eval        # prints every push/pop and final = 36

./minicc --tokens samples/cases/01_arith.mini
```

Implements every requirement of §2.2 (Lab 05) and §2.3 (Extended Token Set) of the project spec.
