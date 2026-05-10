%option noyywrap

%{
#include <stdlib.h>
#include "parse_tree.tab.h"
%}

%%

[ \t]+
[0-9]+      { yylval.ival = atoi(yytext); return NUM; }
\n          { return ENDL; }
.           { return yytext[0]; }

%%
