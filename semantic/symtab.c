#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Scope *g_stack = NULL;
static int g_shadow_counter = 0; /* monotonic suffix for shadow renames */

static Symbol *symbol_new(const char *name, const char *mangled, ValueType t) {
    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    s->name = strdup(name);
    s->mangled = strdup(mangled);
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
        free(sc->syms->mangled);
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

static int name_visible_in_enclosing(const char *name) {
    if (!g_stack)
        return 0;
    for (Scope *sc = g_stack->parent; sc; sc = sc->parent) {
        if (lookup_in_scope(sc, name))
            return 1;
    }
    return 0;
}

Symbol *symtab_insert(const char *name, ValueType t) {
    if (!g_stack)
        symtab_push_scope();
    if (lookup_in_scope(g_stack, name))
        return NULL;
    /* If the same source name exists in any enclosing scope we are
       shadowing it.  Generate a unique mangled name so codegen can
       distinguish the two slots. */
    char mangled[256];
    if (name_visible_in_enclosing(name)) {
        snprintf(mangled, sizeof(mangled), "%s.%d", name, ++g_shadow_counter);
    } else {
        snprintf(mangled, sizeof(mangled), "%s", name);
    }
    Symbol *s = symbol_new(name, mangled, t);
    s->next = g_stack->syms;
    g_stack->syms = s;
    return s;
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
    g_shadow_counter = 0;
}
