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
%}

%union {
    double val;
    char *id;
}

%token <val> NUM
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
    expr '\n' { printf("= %.6g\n", $1); }
    | '\n'
    | error '\n' { yyerrok; }
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
