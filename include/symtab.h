#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"
#include <stdbool.h>

typedef struct Symbol Symbol;
struct Symbol {
    char *name;
    ValueType type;
    Symbol *next;
};

typedef struct Scope Scope;
struct Scope {
    Symbol *syms;
    Scope *parent;
};

void symtab_push_scope(void);
void symtab_pop_scope(void);
Scope *symtab_current(void);

bool symtab_insert(const char *name, ValueType t);
Symbol *symtab_lookup(const char *name);
Symbol *symtab_lookup_local(const char *name);

void symtab_reset(void);

#endif
