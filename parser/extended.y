%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int yylex(void);
void yyerror(const char *s);
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

int main(void) {
    printf("Extended arithmetic: + - * / ^ log() exp() on floating literals.\n");
    return yyparse();
}
