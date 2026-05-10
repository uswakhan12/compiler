CS-346 Mini-Compiler — quick build & run instructions.

Prerequisites
-------------
  * flex
  * bison >= 3.0
      macOS:  `brew install bison`   (Makefile picks up Homebrew's
                                     newer bison automatically)
  * gcc / clang
  * bc (optional, for timing table)

Build
-----
    make

This produces all module binaries at repo root:
  minicc, lab05, postfix_eval,
  postfix, prefix, infix, parse_tree,
  extended, ll1, ll1_generic

Run everything (every module on every test case)
------------------------------------------------
    chmod +x tools/run_all_modules.sh
    ./tools/run_all_modules.sh | tee log.txt

Single-file pipeline
--------------------
    ./minicc --pipeline samples/cases/01_arith.mini
    ./minicc --emit-llvm=demo.ll samples/cases/01_arith.mini
    clang -Wno-override-module demo.ll -o demo_exe && ./demo_exe

Layout (matches the spec's recommended directory structure)
------
    lexer/          Module 1: lexer.l, lab05_lexer.l, postfix_eval.c
    parser/         Modules 2 & 3: postfix/prefix/infix, parse_tree,
                    minicc.y, extended grammar
    first_follow/   Module 4: ll1, ll1_generic, grammars/*.txt
    semantic/       Module 5: AST, symbol table, type & scope checker
    ir/             Module 6: TAC + code generator
    optimizer/      Module 7: optimisation passes (incl. LICM)
    llvm/           Module 8: LLVM IR backend + Clang reference tests
    include/        shared headers
    src/            main.c, token_dump.c
    samples/        test inputs (.mini)
    tools/          run-all / opt-bench scripts
    Makefile
    README.txt

See README.md for the full overview.
