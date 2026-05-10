# Module 3 — Extended Grammar (Functions)

Implements the §4.1 extended grammar:
```
E → E + T | E - T | T
T → T * F | T / F | F
F → B ^ F | B          (right-associative)
B → ( E ) | id | num | log( E ) | exp( E )
```

* Float literals via `strtod`.
* Whitespace and newlines are discarded.
* Right-associativity for `^` declared with `%right '^'`.
* **`YYDEBUG = 1`** is enabled — invoke with `--debug` (or `YYDEBUG=1 env`) to print Bison's full shift/reduce trace, as required by §4.2.
* Math functions linked with `-lm`.

## Build
```
make extended
```

## Run
```
./extended < ../../samples/extended_expr.txt
echo "log(2.0) + exp(1.0)^0.5" | ./extended           # evaluation only
echo "log(10) + 2^3"           | ./extended --debug   # show YYDEBUG trace
```
