# CS-346 Compiler Construction — unified build (Modules 1..8)
#
# Recommended layout (submission / spec):
#   lexer/           Module 1:  Lab 05 baseline lexer, postfix evaluator, lexer.l
#   parser/          Modules 2 & 3: postfix/prefix/infix, parse_tree, minicc.y,
#                    extended grammar (^, log, exp)
#   first_follow/    Module 4:  ll1, ll1_generic, grammars/*.txt
#   semantic/        Module 5:  AST, symbol table, type & scope checker
#   ir/              Module 6:  TAC + code generator
#   optimizer/       Module 7:  optimisation passes (incl. LICM)
#   llvm/            Module 8:  TAC→LLVM backend + Clang reference tests
#
#   include/         shared headers
#   src/             driver (main.c, token_dump.c)
#   samples/         test inputs
#   tools/           run-all / opt-bench scripts
#   build/           generated flex/bison output and .o
#
# Build is portable: macOS uses -ll (Apple flex); Linux uses -lfl.
# On macOS the system bison is 2.3 — the Makefile prefers Homebrew's
# bison (>= 3.0) when installed.

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

BUILD := build
LEX   := lexer
PAR   := parser
FF    := first_follow
SEM   := semantic
IR    := ir
OPT   := optimizer
LLVM  := llvm

.PHONY: all clean dirs calc extended ll1 ll1_generic minicc lab05 \
        postfix_eval parse_tree run-sample run-all

all: dirs minicc calc extended ll1 ll1_generic lab05 postfix_eval parse_tree

dirs:
	mkdir -p $(BUILD) $(LLVM)/out

# ------------------------------------------------------------------ #
# Integrated compiler (Modules 1, 2, 5, 6, 7, 8)                     #
# ------------------------------------------------------------------ #
$(BUILD)/minicc.tab.c $(BUILD)/minicc.tab.h: $(PAR)/minicc.y | dirs
	$(BISON) -d -o $(BUILD)/minicc.tab.c $(PAR)/minicc.y --defines=$(BUILD)/minicc.tab.h

$(BUILD)/lex.minicc.c: $(LEX)/lexer.l $(BUILD)/minicc.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.minicc.c $(LEX)/lexer.l

minicc: $(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
        $(SEM)/ast.c $(SEM)/symtab.c $(SEM)/semantics.c \
        $(IR)/tac.c $(IR)/codegen.c \
        $(OPT)/optimize.c \
        $(LLVM)/llvm_emit.c \
        src/main.c src/token_dump.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o minicc \
		$(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
		$(SEM)/ast.c $(SEM)/symtab.c $(SEM)/semantics.c \
		$(IR)/tac.c $(IR)/codegen.c \
		$(OPT)/optimize.c \
		$(LLVM)/llvm_emit.c \
		src/main.c src/token_dump.c \
		$(LDLIBS)

# ------------------------------------------------------------------ #
# Module 1 — Lab 05 baseline + standalone postfix evaluator          #
# ------------------------------------------------------------------ #
$(BUILD)/lab05.c: $(LEX)/lab05_lexer.l | dirs
	$(FLEX) -o $(BUILD)/lab05.c $(LEX)/lab05_lexer.l

lab05: $(BUILD)/lab05.c
	$(CC) $(CFLAGS) -o lab05 $(BUILD)/lab05.c $(LDLIBS)

postfix_eval: $(LEX)/postfix_eval.c
	$(CC) $(CFLAGS) -o postfix_eval $(LEX)/postfix_eval.c

# ------------------------------------------------------------------ #
# Module 2 — postfix/prefix/infix calculators + parse-tree demo      #
# ------------------------------------------------------------------ #
$(BUILD)/postfix.tab.c $(BUILD)/postfix.tab.h: $(PAR)/postfix.y | dirs
	$(BISON) -d -o $(BUILD)/postfix.tab.c $(PAR)/postfix.y --defines=$(BUILD)/postfix.tab.h

$(BUILD)/lex.postfix.c: $(PAR)/postfix.l $(BUILD)/postfix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.postfix.c $(PAR)/postfix.l

postfix: $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o postfix $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c $(LDLIBS)

$(BUILD)/prefix.tab.c $(BUILD)/prefix.tab.h: $(PAR)/prefix.y | dirs
	$(BISON) -d -o $(BUILD)/prefix.tab.c $(PAR)/prefix.y --defines=$(BUILD)/prefix.tab.h

$(BUILD)/lex.prefix.c: $(PAR)/prefix.l $(BUILD)/prefix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.prefix.c $(PAR)/prefix.l

prefix: $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o prefix $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c $(LDLIBS)

$(BUILD)/infix.tab.c $(BUILD)/infix.tab.h: $(PAR)/infix.y | dirs
	$(BISON) -d -o $(BUILD)/infix.tab.c $(PAR)/infix.y --defines=$(BUILD)/infix.tab.h

$(BUILD)/lex.infix.c: $(PAR)/infix.l $(BUILD)/infix.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.infix.c $(PAR)/infix.l

infix: $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o infix $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c $(LDLIBS)

calc: postfix prefix infix

# Parse-tree / syntax-tree demo with pre/in/post-order traversal.
$(BUILD)/parse_tree.tab.c $(BUILD)/parse_tree.tab.h: $(PAR)/parse_tree.y | dirs
	$(BISON) -d -o $(BUILD)/parse_tree.tab.c $(PAR)/parse_tree.y --defines=$(BUILD)/parse_tree.tab.h

$(BUILD)/lex.parse_tree.c: $(PAR)/parse_tree.l $(BUILD)/parse_tree.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.parse_tree.c $(PAR)/parse_tree.l

parse_tree: $(BUILD)/lex.parse_tree.c $(BUILD)/parse_tree.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o parse_tree \
		$(BUILD)/lex.parse_tree.c $(BUILD)/parse_tree.tab.c $(LDLIBS)

# ------------------------------------------------------------------ #
# Module 3 — extended grammar (^, log, exp) with YYDEBUG enabled     #
# ------------------------------------------------------------------ #
$(BUILD)/extended.tab.c $(BUILD)/extended.tab.h: $(PAR)/extended.y | dirs
	$(BISON) -t -d -o $(BUILD)/extended.tab.c $(PAR)/extended.y --defines=$(BUILD)/extended.tab.h

$(BUILD)/lex.extended.c: $(PAR)/extended.l $(BUILD)/extended.tab.h | dirs
	$(FLEX) -o $(BUILD)/lex.extended.c $(PAR)/extended.l

extended: $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o extended $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c $(LDLIBS)

# ------------------------------------------------------------------ #
# Module 4 — FIRST / FOLLOW / LL(1)                                  #
# ------------------------------------------------------------------ #
ll1: $(FF)/ll1.c
	$(CC) $(CFLAGS) -o ll1 $(FF)/ll1.c

ll1_generic: $(FF)/ll1_generic.c
	$(CC) $(CFLAGS) -o ll1_generic $(FF)/ll1_generic.c

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
