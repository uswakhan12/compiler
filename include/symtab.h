#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"
#include <stdbool.h>

typedef struct Symbol Symbol;
struct Symbol {
    char *name;        /* source name used as the lookup key                */
    char *mangled;     /* emission name: equals `name` for outermost
                          binding, `name.N` for shadows in nested scopes    */
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

/* Insert a declaration in the current scope.
   Returns the new Symbol on success (use `sym->mangled` for TAC emission),
   or NULL if the same source name already exists in the *current* scope.
   If the same source name lives in any *enclosing* scope, the new symbol
   gets a unique mangled name "name.N" so codegen can keep the two slots
   distinct in TAC and LLVM. */
Symbol *symtab_insert(const char *name, ValueType t);

Symbol *symtab_lookup(const char *name);
Symbol *symtab_lookup_local(const char *name);

void symtab_reset(void);

#endif
