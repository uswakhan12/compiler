%code requires {
#include "ast.h"
}

%expect 1

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;

AstNode *g_parse_result = NULL;
%}

%union {
    AstNode *n;
    char *str;
    long long ival;
    double fval;
}

%token INT FLOAT IF ELSE WHILE RETURN LOG EXP
%token <ival> INTEGER_LITERAL
%token <fval> FLOAT_LITERAL
%token <str> IDENTIFIER
%token EQ NE LE GE

%nonassoc UNARY
%left EQ NE
%left '<' '>'
%left LE GE
%left '+' '-'
%left '*' '/'
%right '^'

%type <n> program stmt_list stmt expr block assign_stmt decl_stmt
%type <n> optional_else

%%

program:
    stmt_list { g_parse_result = ast_program($1); }
    ;

stmt_list:
    stmt { $$ = $1; }
    | stmt_list stmt { $$ = ast_stmt_append($1, $2); }
    ;

stmt:
    decl_stmt { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | IF '(' expr ')' stmt optional_else {
        $$ = ast_if($3, $5, $6, yylineno);
    }
    | WHILE '(' expr ')' stmt {
        $$ = ast_while($3, $5, yylineno);
    }
    | block { $$ = $1; }
    | expr ';' { $$ = ast_expr_stmt($1, yylineno); }
    | RETURN expr ';' { $$ = ast_return($2, yylineno); }
    ;

optional_else:
    /* empty */ { $$ = NULL; }
    | ELSE stmt { $$ = $2; }
    ;

decl_stmt:
    INT IDENTIFIER ';' { $$ = ast_decl(TYPE_INT, $2, yylineno); }
    | FLOAT IDENTIFIER ';' { $$ = ast_decl(TYPE_FLOAT, $2, yylineno); }
    ;

assign_stmt:
    IDENTIFIER '=' expr ';' { $$ = ast_assign($1, $3, yylineno); }
    ;

block:
    '{' stmt_list '}' { $$ = ast_block($2, yylineno); }
    | '{' '}' { $$ = ast_block(NULL, yylineno); }
    ;

expr:
    INTEGER_LITERAL { $$ = ast_int_lit($1, yylineno); }
    | FLOAT_LITERAL { $$ = ast_float_lit($1, yylineno); }
    | IDENTIFIER { $$ = ast_var($1, yylineno); }
    | expr '+' expr { $$ = ast_binop(AST_OP_ADD, $1, $3, yylineno); }
    | expr '-' expr { $$ = ast_binop(AST_OP_SUB, $1, $3, yylineno); }
    | expr '*' expr { $$ = ast_binop(AST_OP_MUL, $1, $3, yylineno); }
    | expr '/' expr { $$ = ast_binop(AST_OP_DIV, $1, $3, yylineno); }
    | expr '^' expr { $$ = ast_binop(AST_OP_POW, $1, $3, yylineno); }
    | expr EQ expr { $$ = ast_binop(AST_OP_EQ, $1, $3, yylineno); }
    | expr NE expr { $$ = ast_binop(AST_OP_NE, $1, $3, yylineno); }
    | expr '<' expr { $$ = ast_binop(AST_OP_LT, $1, $3, yylineno); }
    | expr '>' expr { $$ = ast_binop(AST_OP_GT, $1, $3, yylineno); }
    | expr LE expr { $$ = ast_binop(AST_OP_LE, $1, $3, yylineno); }
    | expr GE expr { $$ = ast_binop(AST_OP_GE, $1, $3, yylineno); }
    | '-' expr %prec UNARY { $$ = ast_unary('-', $2, yylineno); }
    | '(' expr ')' { $$ = $2; }
    | LOG '(' expr ')' { $$ = ast_call(1, $3, yylineno); }
    | EXP '(' expr ')' { $$ = ast_call(0, $3, yylineno); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "parse error at line %d: %s\n", yylineno, s);
}
