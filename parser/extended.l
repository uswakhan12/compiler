%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extended.tab.h"
%}

%%

[ \t]+
\n                      { return '\n'; }

[0-9]+"."[0-9]+         { yylval.val = strtod(yytext, NULL); return NUM; }
[0-9]+                  { yylval.val = strtod(yytext, NULL); return NUM; }

"log"                   { return LOG; }
"exp"                   { return EXP; }

"("                     { return '('; }
")"                     { return ')'; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"^"                     { return '^'; }

.                       { fprintf(stderr, "illegal char %s\n", yytext); return 0; }

%%
