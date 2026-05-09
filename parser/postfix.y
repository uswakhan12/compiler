%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);
void yyerror(const char *s);

#define STK 256
static int stack[STK];
static int sp;

static void push(int v) {
    if (sp >= STK) {
        fprintf(stderr, "stack overflow\n");
        exit(1);
    }
    printf("push %d (sp=%d)\n", v, sp);
    stack[sp++] = v;
}

static int pop(void) {
    if (sp <= 0) {
        fprintf(stderr, "stack underflow\n");
        exit(1);
    }
    int v = stack[--sp];
    printf("pop -> %d (sp=%d)\n", v, sp);
    return v;
}

static int top(void) { return sp > 0 ? stack[sp - 1] : 0; }
%}

%union { int ival; }
%token <ival> NUM
%token ENDL

%%

lines:
    /* empty */
    | lines line
    ;

line:
    rpn ENDL { printf("Final result: %d\n\n", top()); sp = 0; }
    ;

rpn:
    rpn NUM { push($2); }
    | rpn '+' { int b = pop(); int a = pop(); push(a + b); printf("apply +\n"); }
    | rpn '-' { int b = pop(); int a = pop(); push(a - b); printf("apply -\n"); }
    | rpn '*' { int b = pop(); int a = pop(); push(a * b); printf("apply *\n"); }
    | NUM { push($1); }
    ;

%%

void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }

int main(void) {
    printf("Postfix calculator — enter integers and + - * (space-separated), end line with Enter.\n");
    return yyparse();
}
