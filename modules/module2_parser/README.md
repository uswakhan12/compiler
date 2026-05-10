# Module 2 — Syntax Analysis (Parsing)

Three calculator variants plus an explicit parse-tree builder that prints
pre-/in-/post-order traversals.

| File | Purpose |
|------|---------|
| `postfix.l` / `postfix.y` | Postfix calculator with stack-transition output (push/pop). |
| `prefix.l` / `prefix.y`   | Prefix calculator, recursive evaluation. |
| `infix.l` / `infix.y`     | Infix calculator with operator precedence and associativity declarations (`%left +/-`, `%left * /`, `%right ^`). |
| `parse_tree.l` / `parse_tree.y` | Builds an **explicit parse tree**, derives the compact **syntax tree**, and prints **pre-order (prefix)**, **in-order (infix)**, and **post-order (postfix)** traversals — directly addressing §3.3 of the spec. |
| `minicc.y`                | Integrated parser for the mini language (used by `minicc`). |

## Build
```
make calc parse_tree minicc
```

## Run
```
echo "4 8 + 7 *"  | ./postfix
echo "* 3 + 4 5"  | ./prefix
echo "(1+2)*3-4"  | ./infix
echo "1+2*(3+4)"  | ./parse_tree   # parse tree + syntax tree + 3 traversals
```
