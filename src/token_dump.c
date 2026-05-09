#include "token_dump.h"

#include <stdio.h>

#include "minicc.tab.h"

extern int yylex(void);
extern YYSTYPE yylval;
void yyrestart(FILE *f);

void print_token_stream(FILE *in) {
    yyrestart(in);
    printf("--- Token stream ---\n");
    int t;
    while ((t = yylex()) != 0) {
        switch (t) {
        case INT:
            printf("INT\n");
            break;
        case FLOAT:
            printf("FLOAT\n");
            break;
        case IF:
            printf("IF\n");
            break;
        case ELSE:
            printf("ELSE\n");
            break;
        case WHILE:
            printf("WHILE\n");
            break;
        case RETURN:
            printf("RETURN\n");
            break;
        case LOG:
            printf("LOG\n");
            break;
        case EXP:
            printf("EXP\n");
            break;
        case INTEGER_LITERAL:
            printf("INTEGER_LITERAL\t%lld\n", (long long)yylval.ival);
            break;
        case FLOAT_LITERAL:
            printf("FLOAT_LITERAL\t%.6g\n", yylval.fval);
            break;
        case IDENTIFIER:
            printf("IDENTIFIER\t%s\n", yylval.str);
            break;
        case EQ:
            printf("EQ\t==\n");
            break;
        case NE:
            printf("NE\t!=\n");
            break;
        case LE:
            printf("LE\t<=\n");
            break;
        case GE:
            printf("GE\t>=\n");
            break;
        default:
            if (t > 0 && t < 256)
                printf("CHAR\t'%c'\n", (char)t);
            else
                printf("TOKEN\t%d\n", t);
            break;
        }
    }
    printf("--- end of stream ---\n");
}
