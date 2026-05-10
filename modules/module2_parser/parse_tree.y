/*
 * Module 2 — Parse-tree / syntax-tree builder with pre/in/post-order
 * traversals.
 *
 * Demonstrates the difference between a *parse tree* (records every
 * production used during derivation, e.g. extra E/T/F nodes) and a
 * *syntax tree* (compact: only operator + operands). Each input
 * expression prints:
 *   - PARSE TREE     (verbose, with intermediate non-terminals)
 *   - SYNTAX TREE
 *   - Pre-order      (prefix notation)
 *   - In-order       (infix notation)
 *   - Post-order     (postfix notation)
 *
 * Build:  see Makefile target `parse_tree`.
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int yylex(void);
void yyerror(const char *s);

typedef struct Node Node;
struct Node {
    char *label;     /* operator string or non-terminal name */
    int   is_leaf;   /* 1 = number leaf (use ival) */
    int   ival;
    Node *kids[8];
    int   nkids;
};

static Node *mknode(const char *label) {
    Node *n = (Node *)calloc(1, sizeof(Node));
    n->label = strdup(label);
    return n;
}
static Node *mknum(int v) {
    Node *n = mknode("NUM");
    n->is_leaf = 1; n->ival = v;
    return n;
}
static Node *add_kid(Node *p, Node *c) {
    if (c && p->nkids < 8) p->kids[p->nkids++] = c;
    return p;
}

/* ---------- Parse-tree print (verbose, indented) ---------- */
static void print_parse_tree(Node *n, int depth) {
    if (!n) return;
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
    if (n->is_leaf) printf("NUM(%d)\n", n->ival);
    else printf("%s\n", n->label);
    for (int i = 0; i < n->nkids; i++) print_parse_tree(n->kids[i], depth + 1);
}

/* ---------- Syntax-tree (drop chain nodes) ---------- */
static Node *to_syntax_tree(Node *n) {
    if (!n) return NULL;
    if (n->is_leaf) return n;
    /* If this is a non-terminal chain (single child), skip it. */
    if (n->nkids == 1 && n->label[0] != '+' && n->label[0] != '-' &&
        n->label[0] != '*' && n->label[0] != '/') {
        return to_syntax_tree(n->kids[0]);
    }
    Node *out = mknode(n->label);
    for (int i = 0; i < n->nkids; i++) {
        Node *c = to_syntax_tree(n->kids[i]);
        if (c) add_kid(out, c);
    }
    return out;
}
static void print_syntax_tree(Node *n, int depth) {
    if (!n) return;
    for (int i = 0; i < depth; i++) fputs("  ", stdout);
    if (n->is_leaf) printf("%d\n", n->ival);
    else printf("%s\n", n->label);
    for (int i = 0; i < n->nkids; i++) print_syntax_tree(n->kids[i], depth + 1);
}

/* ---------- Traversals on syntax tree ---------- */
static void preorder(Node *n) {
    if (!n) return;
    if (n->is_leaf) printf("%d ", n->ival);
    else printf("%s ", n->label);
    for (int i = 0; i < n->nkids; i++) preorder(n->kids[i]);
}
static void inorder(Node *n) {
    if (!n) return;
    if (n->is_leaf) { printf("%d ", n->ival); return; }
    if (n->nkids == 2) {
        printf("( "); inorder(n->kids[0]);
        printf("%s ", n->label);
        inorder(n->kids[1]); printf(") ");
    } else {
        printf("%s ", n->label);
        for (int i = 0; i < n->nkids; i++) inorder(n->kids[i]);
    }
}
static void postorder(Node *n) {
    if (!n) return;
    for (int i = 0; i < n->nkids; i++) postorder(n->kids[i]);
    if (n->is_leaf) printf("%d ", n->ival);
    else printf("%s ", n->label);
}

static Node *g_root = NULL;
%}

%union { int ival; struct Node *node; }
%token <ival> NUM
%token ENDL
%type <node> E T F

%%

lines:
    /* empty */
    | lines line
    ;

line:
    E ENDL {
        printf("\n=== PARSE TREE (verbose) ===\n");
        print_parse_tree($1, 0);
        Node *st = to_syntax_tree($1);
        printf("=== SYNTAX TREE (compact) ===\n");
        print_syntax_tree(st, 0);
        printf("Pre-order  (prefix) : "); preorder(st);  printf("\n");
        printf("In-order   (infix)  : "); inorder(st);   printf("\n");
        printf("Post-order (postfix): "); postorder(st); printf("\n\n");
        g_root = NULL;
    }
    | ENDL
    ;

E:
    E '+' T { Node *n = mknode("+"); add_kid(n,$1); add_kid(n,$3); $$ = n; }
  | E '-' T { Node *n = mknode("-"); add_kid(n,$1); add_kid(n,$3); $$ = n; }
  | T       { Node *n = mknode("E"); add_kid(n,$1); $$ = n; }
  ;

T:
    T '*' F { Node *n = mknode("*"); add_kid(n,$1); add_kid(n,$3); $$ = n; }
  | T '/' F { Node *n = mknode("/"); add_kid(n,$1); add_kid(n,$3); $$ = n; }
  | F       { Node *n = mknode("T"); add_kid(n,$1); $$ = n; }
  ;

F:
    '(' E ')' { Node *n = mknode("F"); add_kid(n,$2); $$ = n; }
  | NUM       { Node *n = mknode("F"); add_kid(n, mknum($1)); $$ = n; }
  ;

%%

void yyerror(const char *s) { fprintf(stderr, "parse error: %s\n", s); }

int main(void) {
    printf("Module 2 parse-tree demo. Type an expression and press Enter:\n");
    return yyparse();
}
