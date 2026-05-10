%option noyywrap

%{
#include <stdlib.h>
#include "infix.tab.h"
%}

%%

[ \t]+
[0-9]+                  { yylval.ival = atoi(yytext); return NUM; }
\n                      { return ENDL; }
.                       { return yytext[0]; }

%%
