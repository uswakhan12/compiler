/*
 * Module 1 — Lab 05 Task 1 baseline lexer.
 *
 * Implements the exact requirements from the Lab 05 specification:
 *   - integer literals
 *   - floating-point literals
 *   - identifiers: must start with a LOWERCASE letter, digits allowed
 *     in non-initial positions
 *   - keywords: if, then, begin, end, procedure, function
 *   - arithmetic operators: + - * /
 *   - whitespace is skipped
 *   - any unrecognised character produces an error message
 *
 * This is intentionally separate from the full pipeline lexer
 * (see `lexer.l`) which extends Lab 05 with the additional token set
 * required for the integrated compiler.
 *
 * Build:  flex -o lab05.c lab05_lexer.l
 *         gcc  -o lab05 lab05.c -ll        (macOS)  or  -lfl (Linux)
 * Run:    ./lab05 < some_input.txt
 */
%option noyywrap
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

DIGIT      [0-9]
INT_LIT    {DIGIT}+
FLOAT_LIT  {DIGIT}+\.{DIGIT}+
IDENT      [a-z][a-z0-9]*

%%

[ \t\r\n]+               { /* whitespace */ }

"if"                     { printf("KEYWORD\tif\n"); }
"then"                   { printf("KEYWORD\tthen\n"); }
"begin"                  { printf("KEYWORD\tbegin\n"); }
"end"                    { printf("KEYWORD\tend\n"); }
"procedure"              { printf("KEYWORD\tprocedure\n"); }
"function"               { printf("KEYWORD\tfunction\n"); }

{FLOAT_LIT}              { printf("FLOAT\t%s\n", yytext); }
{INT_LIT}                { printf("INT\t%s\n", yytext); }
{IDENT}                  { printf("IDENT\t%s\n", yytext); }

"+"                      { printf("OP\t+\n"); }
"-"                      { printf("OP\t-\n"); }
"*"                      { printf("OP\t*\n"); }
"/"                      { printf("OP\t/\n"); }

.                        {
                            fprintf(stderr, "lexer error: unrecognised character '%s' at line %d\n",
                                    yytext, yylineno);
                         }

%%

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { perror(argv[1]); return 1; }
        yyin = f;
    }
    yylex();
    return 0;
}
