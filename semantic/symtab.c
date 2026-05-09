#include "symtab.h"
#include <stdlib.h>
#include <string.h>

static Scope *g_stack = NULL;

static Symbol *symbol_new(const char *name, ValueType t) {
    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    s->name = strdup(name);
    s->type = t;
    s->next = NULL;
    return s;
}

void symtab_push_scope(void) {
    Scope *sc = (Scope *)malloc(sizeof(Scope));
    sc->syms = NULL;
    sc->parent = g_stack;
    g_stack = sc;
}

void symtab_pop_scope(void) {
    if (!g_stack)
        return;
    Scope *sc = g_stack;
    g_stack = g_stack->parent;
    while (sc->syms) {
        Symbol *nx = sc->syms->next;
        free(sc->syms->name);
        free(sc->syms);
        sc->syms = nx;
    }
    free(sc);
}

Scope *symtab_current(void) { return g_stack; }

static Symbol *lookup_in_scope(Scope *sc, const char *name) {
    for (Symbol *s = sc ? sc->syms : NULL; s; s = s->next) {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

bool symtab_insert(const char *name, ValueType t) {
    if (!g_stack)
        symtab_push_scope();
    if (lookup_in_scope(g_stack, name))
        return false;
    Symbol *s = symbol_new(name, t);
    s->next = g_stack->syms;
    g_stack->syms = s;
    return true;
}

Symbol *symtab_lookup(const char *name) {
    for (Scope *sc = g_stack; sc; sc = sc->parent) {
        Symbol *s = lookup_in_scope(sc, name);
        if (s)
            return s;
    }
    return NULL;
}

Symbol *symtab_lookup_local(const char *name) {
    return lookup_in_scope(g_stack, name);
}

void symtab_reset(void) {
    while (g_stack)
        symtab_pop_scope();
}
