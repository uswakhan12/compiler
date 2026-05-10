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

Layout
------
    modules/module1_lexer/        (Module 1)
    modules/module2_parser/       (Module 2)
    modules/module3_extended/     (Module 3)
    modules/module4_first_follow/ (Module 4)
    modules/module5_semantics/    (Module 5)
    modules/module6_ir/           (Module 6)
    modules/module7_optimizer/    (Module 7)
    modules/module8_llvm/         (Module 8)
    include/                      (shared headers)
    src/                          (main.c, token_dump.c)
    samples/                      (test inputs)
    tools/                        (run-all / opt-bench scripts)

Each module folder has its own README.md describing what it implements
and how to run its tool.

See README.md for the full overview.
