%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);
void yyerror(const char *s);
%}

%union { int ival; }
%token <ival> NUM
%token ENDL

%type <ival> expr line

%left '+' '-'
%left '*' '/'
%right '^'

%%

lines:
    /* empty */
    | lines line
    ;

line:
    expr ENDL { printf("Infix result: %d\n\n", $1); }
    ;

expr:
    expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | expr '*' expr { $$ = $1 * $3; }
    | expr '/' expr { $$ = $1 / $3; }
    | expr '^' expr { int b = (int)$3, a = (int)$1, r = 1, i; for (i = 0; i < b; i++) r *= a; $$ = r; }
    | '(' expr ')' { $$ = $2; }
    | NUM { $$ = $1; }
    ;

%%

void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }

int main(void) {
    printf("Infix calculator — integers, + - * / ^ and parentheses.\n");
    return yyparse();
}
