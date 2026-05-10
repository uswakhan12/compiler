%option noyywrap

%{
#include <stdlib.h>
#include "postfix.tab.h"
%}

%%

[ \t]+
[0-9]+                  { yylval.ival = atoi(yytext); return NUM; }
"+"|"-"|"*"|"/"|"^"     { return yytext[0]; }
\n                      { return ENDL; }
.                       { fprintf(stderr, "bad token %s\n", yytext); return 0; }

%%
