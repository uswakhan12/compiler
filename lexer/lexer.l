%option noyywrap
%option yylineno

%{
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "minicc.tab.h"

void yyerror(const char *s);
static int comment_depth = 0;
%}

%x COMMENT

%%

[ \t\r\n]+

"//".*

"/*"                    { comment_depth = 1; BEGIN(COMMENT); }
<COMMENT>"/*"           { comment_depth++; }
<COMMENT>"*/"           { comment_depth--; if (comment_depth == 0) BEGIN(0); }
<COMMENT>.|\n           { /* skip */ }

"int"                   { return INT; }
"float"                 { return FLOAT; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"return"                { return RETURN; }
"log"                   { return LOG; }
"exp"                   { return EXP; }

"=="                    { return EQ; }
"!="                    { return NE; }
"<="                    { return LE; }
">="                    { return GE; }
"="                     { return '='; }

[A-Za-z_][A-Za-z0-9_]*  { yylval.str = strdup(yytext); return IDENTIFIER; }

[0-9]+"."[0-9]+         { yylval.fval = strtod(yytext, NULL); return FLOAT_LITERAL; }
[0-9]+                  { yylval.ival = atoll(yytext); return INTEGER_LITERAL; }

"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"^"                     { return '^'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
","                     { return ','; }
"{"                     { return '{'; }
"}"                     { return '}'; }
"<"                     { return '<'; }
">"                     { return '>'; }

.                       {
    fprintf(stderr, "lexer error at line %d: unrecognized character '%s'\n", yylineno, yytext);
    return 0;
}

%%
