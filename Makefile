# CS-346 Compiler Construction — unified build (Modules 1..8)
#
# Layout
#   modules/module1_lexer/        Lab 05 baseline lexer, postfix evaluator,
#                                 full-project lexer (lexer.l)
#   modules/module2_parser/       Postfix / Prefix / Infix calculators,
#                                 parse-tree-with-traversals demo,
#                                 minicc.y (integrated parser grammar)
#   modules/module3_extended/     Extended grammar (^, log, exp) + YYDEBUG
#   modules/module4_first_follow/ Fixed-grammar LL(1) tool + generic LL(1) tool
#   modules/module5_semantics/    Symbol table, AST helpers, type/scope checker
#   modules/module6_ir/           TAC IR + code generator
#   modules/module7_optimizer/    All optimisation passes incl. LICM (Task 5)
#   modules/module8_llvm/         TAC→LLVM-IR backend + Clang reference tests
#
#   include/                      shared headers
#   src/                          driver (main.c, token_dump.c)
#   samples/                      .mini test cases + bad_*.mini failure demos
#   tools/                        run-all / opt-bench shell scripts
#   build/                        generated artifacts (lex/yacc output, .o)
#
# Build is portable: macOS uses -ll (Apple flex); Linux uses -lfl.
# On macOS the system bison is 2.3 and predates the syntax used by some
# of the .y files, so the Makefile prefers Homebrew's newer bison if it
# is installed.

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O2
CPPFLAGS = -Iinclude -Ibuild
LDFLAGS  =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDLIBS    = -ll -lm
HBREW_BIN := /opt/homebrew/opt/bison/bin
ifeq ($(wildcard $(HBREW_BIN)/bison),$(HBREW_BIN)/bison)
BISON := $(HBREW_BIN)/bison
else
BISON := bison
endif
else
LDLIBS = -lfl -lm
BISON  := bison
endif

FLEX  := flex

BUILD  := build

M1     := modules/module1_lexer
M2     := modules/module2_parser
M3     := modules/module3_extended
M4     := modules/module4_first_follow
M5     := modules/module5_semantics
M6     := modules/module6_ir
M7     := modules/module7_optimizer
M8     := modules/module8_llvm

.PHONY: all clean dirs calc extended ll1 ll1_generic minicc lab05 \
        postfix_eval parse_tree run-sample run-all

all: dirs minicc calc extended ll1 ll1_generic lab05 postfix_eval parse_tree

dirs:
	mkdir -p $(BUILD) $(M8)/out

# ------------------------------------------------------------------ #
# Integrated compiler (Modules 1, 2, 5, 6, 7, 8)                     #
# ------------------------------------------------------------------ #
$(BUILD)/minicc.tab.c $(BUILD)/minicc.tab.h: $(M2)/minicc.y | dirs
	$(BISON) -d -o $(BUILD)/minicc.tab.c $(M2)/minicc.y --defines=$(BUILD)/minicc.tab.h

$(BUILD)/lex.minicc.c: $(M1)/lexer.l $(BUILD)/minicc.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.minicc.c $(M1)/lexer.l

minicc: $(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
        $(M5)/ast.c $(M5)/symtab.c $(M5)/semantics.c \
        $(M6)/tac.c $(M6)/codegen.c \
        $(M7)/optimize.c \
        $(M8)/llvm_emit.c \
        src/main.c src/token_dump.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o minicc \
		$(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
		$(M5)/ast.c $(M5)/symtab.c $(M5)/semantics.c \
		$(M6)/tac.c $(M6)/codegen.c \
		$(M7)/optimize.c \
		$(M8)/llvm_emit.c \
		src/main.c src/token_dump.c \
		$(LDLIBS)

# ------------------------------------------------------------------ #
# Module 1 — Lab 05 baseline + standalone postfix evaluator          #
# ------------------------------------------------------------------ #
$(BUILD)/lab05.c: $(M1)/lab05_lexer.l | dirs
	$(FLEX) -o $(BUILD)/lab05.c $(M1)/lab05_lexer.l

lab05: $(BUILD)/lab05.c
	$(CC) $(CFLAGS) -o lab05 $(BUILD)/lab05.c $(LDLIBS)

postfix_eval: $(M1)/postfix_eval.c
	$(CC) $(CFLAGS) -o postfix_eval $(M1)/postfix_eval.c

# ------------------------------------------------------------------ #
# Module 2 — postfix/prefix/infix calculators + parse-tree demo      #
# ------------------------------------------------------------------ #
$(BUILD)/postfix.tab.c $(BUILD)/postfix.tab.h: $(M2)/postfix.y | dirs
	$(BISON) -d -o $(BUILD)/postfix.tab.c $(M2)/postfix.y --defines=$(BUILD)/postfix.tab.h

$(BUILD)/lex.postfix.c: $(M2)/postfix.l $(BUILD)/postfix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.postfix.c $(M2)/postfix.l

postfix: $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o postfix $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c $(LDLIBS)

$(BUILD)/prefix.tab.c $(BUILD)/prefix.tab.h: $(M2)/prefix.y | dirs
	$(BISON) -d -o $(BUILD)/prefix.tab.c $(M2)/prefix.y --defines=$(BUILD)/prefix.tab.h

$(BUILD)/lex.prefix.c: $(M2)/prefix.l $(BUILD)/prefix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.prefix.c $(M2)/prefix.l

prefix: $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o prefix $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c $(LDLIBS)

$(BUILD)/infix.tab.c $(BUILD)/infix.tab.h: $(M2)/infix.y | dirs
	$(BISON) -d -o $(BUILD)/infix.tab.c $(M2)/infix.y --defines=$(BUILD)/infix.tab.h

$(BUILD)/lex.infix.c: $(M2)/infix.l $(BUILD)/infix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.infix.c $(M2)/infix.l

infix: $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o infix $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c $(LDLIBS)

calc: postfix prefix infix

# Parse-tree / syntax-tree demo with pre/in/post-order traversal.
$(BUILD)/parse_tree.tab.c $(BUILD)/parse_tree.tab.h: $(M2)/parse_tree.y | dirs
	$(BISON) -d -o $(BUILD)/parse_tree.tab.c $(M2)/parse_tree.y --defines=$(BUILD)/parse_tree.tab.h

$(BUILD)/lex.parse_tree.c: $(M2)/parse_tree.l $(BUILD)/parse_tree.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.parse_tree.c $(M2)/parse_tree.l

parse_tree: $(BUILD)/lex.parse_tree.c $(BUILD)/parse_tree.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o parse_tree \
		$(BUILD)/lex.parse_tree.c $(BUILD)/parse_tree.tab.c $(LDLIBS)

# ------------------------------------------------------------------ #
# Module 3 — extended grammar (^, log, exp) with YYDEBUG enabled     #
# ------------------------------------------------------------------ #
$(BUILD)/extended.tab.c $(BUILD)/extended.tab.h: $(M3)/extended.y | dirs
	$(BISON) -t -d -o $(BUILD)/extended.tab.c $(M3)/extended.y --defines=$(BUILD)/extended.tab.h

$(BUILD)/lex.extended.c: $(M3)/extended.l $(BUILD)/extended.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.extended.c $(M3)/extended.l

extended: $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o extended $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c $(LDLIBS)

# ------------------------------------------------------------------ #
# Module 4 — FIRST / FOLLOW / LL(1)                                  #
# ------------------------------------------------------------------ #
ll1: $(M4)/ll1.c
	$(CC) $(CFLAGS) -o ll1 $(M4)/ll1.c

ll1_generic: $(M4)/ll1_generic.c
	$(CC) $(CFLAGS) -o ll1_generic $(M4)/ll1_generic.c

# ------------------------------------------------------------------ #
# Convenience targets                                                #
# ------------------------------------------------------------------ #
run-sample: minicc
	./minicc samples/demo.mini

run-all: all
	chmod +x tools/run_all_modules.sh
	./tools/run_all_modules.sh

clean:
	rm -rf $(BUILD) minicc postfix prefix infix extended ll1 ll1_generic \
	       lab05 postfix_eval parse_tree
