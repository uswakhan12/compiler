# CS-346 Compiler Construction — unified build
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
CPPFLAGS = -Iinclude -Ibuild
LDFLAGS =
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDLIBS  = -ll -lm
else
LDLIBS  = -lfl -lm
endif

BUILD   = build

.PHONY: all clean dirs calc extended ll1 minicc run-sample run-all

all: dirs minicc calc extended ll1

dirs:
	mkdir -p $(BUILD)

# --- Integrated compiler (Modules 1,2,3,5,6,7) ---
$(BUILD)/minicc.tab.c $(BUILD)/minicc.tab.h: parser/minicc.y | dirs
	bison -d -o $(BUILD)/minicc.tab.c parser/minicc.y --defines=$(BUILD)/minicc.tab.h

$(BUILD)/lex.minicc.c: lexer/lexer.l $(BUILD)/minicc.tab.h | dirs
	flex -o $(BUILD)/lex.minicc.c lexer/lexer.l

minicc: $(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
		semantic/ast.c semantic/symtab.c semantic/semantics.c \
		ir/tac.c ir/codegen.c optimizer/optimize.c src/main.c src/token_dump.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o minicc \
		$(BUILD)/lex.minicc.c $(BUILD)/minicc.tab.c \
		semantic/ast.c semantic/symtab.c semantic/semantics.c \
		ir/tac.c ir/codegen.c optimizer/optimize.c src/main.c src/token_dump.c \
		$(LDLIBS)

# --- Module 2: postfix / prefix / infix calculators ---
$(BUILD)/postfix.tab.c $(BUILD)/postfix.tab.h: parser/postfix.y | dirs
	bison -d -o $(BUILD)/postfix.tab.c parser/postfix.y --defines=$(BUILD)/postfix.tab.h

$(BUILD)/lex.postfix.c: parser/postfix.l $(BUILD)/postfix.tab.h | dirs
	flex -o $(BUILD)/lex.postfix.c parser/postfix.l

postfix: $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o postfix $(BUILD)/lex.postfix.c $(BUILD)/postfix.tab.c $(LDLIBS)

$(BUILD)/prefix.tab.c $(BUILD)/prefix.tab.h: parser/prefix.y | dirs
	bison -d -o $(BUILD)/prefix.tab.c parser/prefix.y --defines=$(BUILD)/prefix.tab.h

$(BUILD)/lex.prefix.c: parser/prefix.l $(BUILD)/prefix.tab.h | dirs
	flex -o $(BUILD)/lex.prefix.c parser/prefix.l

prefix: $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o prefix $(BUILD)/lex.prefix.c $(BUILD)/prefix.tab.c $(LDLIBS)

$(BUILD)/infix.tab.c $(BUILD)/infix.tab.h: parser/infix.y | dirs
	bison -d -o $(BUILD)/infix.tab.c parser/infix.y --defines=$(BUILD)/infix.tab.h

$(BUILD)/lex.infix.c: parser/infix.l $(BUILD)/infix.tab.h | dirs
	flex -o $(BUILD)/lex.infix.c parser/infix.l

infix: $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o infix $(BUILD)/lex.infix.c $(BUILD)/infix.tab.c $(LDLIBS)

calc: postfix prefix infix

# --- Module 3: extended grammar (floats, ^, log, exp) ---
$(BUILD)/extended.tab.c $(BUILD)/extended.tab.h: parser/extended.y | dirs
	bison -d -o $(BUILD)/extended.tab.c parser/extended.y --defines=$(BUILD)/extended.tab.h

$(BUILD)/lex.extended.c: parser/extended.l $(BUILD)/extended.tab.h | dirs
	flex -o $(BUILD)/lex.extended.c parser/extended.l

extended: $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c
	$(CC) $(CFLAGS) -I$(BUILD) -o extended $(BUILD)/lex.extended.c $(BUILD)/extended.tab.c $(LDLIBS)

# --- Module 4: FIRST / FOLLOW / LL(1) ---
ll1: first_follow/ll1.c
	$(CC) $(CFLAGS) -o ll1 first_follow/ll1.c

run-sample: minicc
	./minicc samples/demo.mini

run-all: all
	chmod +x tools/run_all_modules.sh
	./tools/run_all_modules.sh

clean:
	rm -rf $(BUILD) minicc postfix prefix infix extended ll1
