%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int yylex(void);
void yyerror(const char *s);

/* Module 3 — enable Bison debug traces (required by the project spec).
   Setting `yydebug = 1` at runtime causes Bison to print every shift /
   reduce / token-lookahead it performs.  Override with the environment
   variable YYDEBUG=0 to silence. */
#define YYDEBUG 1
extern int yydebug;

/* Tiny linear variable table so the extended grammar can support
   the `id` rule (spec §4.1: B → ( E ) | id | num | log(E) | exp(E)).
   `x = 2*3 + 1` binds x; using `x` in a later expression evaluates
   to that bound value.  Unknown ids print a warning and evaluate to 0. */
typedef struct VarBind {
    char *name;
    double value;
    struct VarBind *next;
} VarBind;
static VarBind *g_vars = NULL;

static double var_lookup(const char *name) {
    for (VarBind *v = g_vars; v; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v->value;
    fprintf(stderr, "warning: undefined identifier '%s' — using 0\n", name);
    return 0.0;
}

static void var_set(const char *name, double v) {
    for (VarBind *p = g_vars; p; p = p->next)
        if (strcmp(p->name, name) == 0) { p->value = v; return; }
    VarBind *b = (VarBind *)malloc(sizeof(VarBind));
    b->name = strdup(name);
    b->value = v;
    b->next = g_vars;
    g_vars = b;
}
%}

%union {
    double val;
    char *id;
}

%token <val> NUM
%token <id>  ID
%token LOG EXP

%left '+' '-'
%left '*' '/'
%right '^'

%type <val> expr

%%

input:
    | input line
    ;

line:
    expr '\n'         { printf("= %.6g\n", $1); }
    | ID '=' expr '\n' { var_set($1, $3); printf("%s := %.6g\n", $1, $3); free($1); }
    | '\n'
    | error '\n'      { yyerrok; }
    ;

expr:
    expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | expr '*' expr { $$ = $1 * $3; }
    | expr '/' expr { $$ = $1 / $3; }
    | expr '^' expr { $$ = pow($1, $3); }
    | '(' expr ')'  { $$ = $2; }
    | LOG '(' expr ')' { $$ = log($3); }
    | EXP '(' expr ')' { $$ = exp($3); }
    | NUM { $$ = $1; }
    | ID  { $$ = var_lookup($1); free($1); }
    ;

%%

void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }

int main(int argc, char **argv) {
    /* `--debug` or env YYDEBUG=1 turns on Bison's shift/reduce traces. */
    const char *env = getenv("YYDEBUG");
    if (env && env[0] == '1') yydebug = 1;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--debug") == 0) yydebug = 1;
    printf("Extended arithmetic: + - * / ^ log() exp() on floating literals.\n");
    if (yydebug) printf("(YYDEBUG enabled — Bison trace will follow)\n");
    return yyparse();
}
