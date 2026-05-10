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

%%

lines:
    /* empty */
    | lines line
    ;

line:
    expr ENDL { printf("Prefix result: %d\n\n", $1); }
    ;

expr:
    '+' expr expr { $$ = $2 + $3; printf("+ %d %d -> %d\n", $2, $3, $$); }
    | '-' expr expr { $$ = $2 - $3; printf("- %d %d -> %d\n", $2, $3, $$); }
    | '*' expr expr { $$ = $2 * $3; printf("* %d %d -> %d\n", $2, $3, $$); }
    | '/' expr expr { $$ = ($3 == 0) ? 0 : $2 / $3; printf("/ %d %d -> %d\n", $2, $3, $$); }
    | '^' expr expr { int r = 1; for (int i = 0; i < $3; i++) r *= $2; $$ = r; printf("^ %d %d -> %d\n", $2, $3, $$); }
    | NUM { $$ = $1; }
    ;

%%

void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }

int main(void) {
    printf("Prefix calculator (+ - * / ^ with prefix notation, e.g. + 4 8).\n");
    return yyparse();
}
