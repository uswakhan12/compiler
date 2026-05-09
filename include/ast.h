#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_UNKNOWN
} ValueType;

typedef enum {
    AST_OP_ADD,
    AST_OP_SUB,
    AST_OP_MUL,
    AST_OP_DIV,
    AST_OP_POW,
    AST_OP_EQ,
    AST_OP_NE,
    AST_OP_LT,
    AST_OP_GT,
    AST_OP_LE,
    AST_OP_GE
} AstBinOp;

typedef enum {
    AST_PROGRAM,
    AST_DECL,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_BLOCK,
    AST_EXPR_STMT,
    AST_RETURN,
    AST_BINOP,
    AST_UNARY,
    AST_CALL,
    AST_VAR,
    AST_INT_LIT,
    AST_FLOAT_LIT
} AstKind;

typedef struct AstNode AstNode;

struct AstNode {
    AstKind kind;
    ValueType inferred_type;
    int line;
    AstNode *next;
    union {
        struct {
            ValueType decl_type;
            char *name;
        } decl;
        struct {
            char *name;
            AstNode *expr;
        } assign;
        struct {
            AstNode *cond;
            AstNode *then_stmt;
            AstNode *else_stmt;
        } iff;
        struct {
            AstNode *cond;
            AstNode *body;
        } whloop;
        struct {
            AstNode *first;
        } block;
        struct {
            AstNode *expr;
        } expr_stmt;
        struct {
            AstNode *expr;
        } ret;
        struct {
            AstBinOp op;
            AstNode *left;
            AstNode *right;
        } binop;
        struct {
            int op;
            AstNode *child;
        } unary;
        struct {
            int is_log;
            AstNode *arg;
        } call;
        struct {
            char *name;
        } var;
        struct {
            long long value;
        } int_lit;
        struct {
            double value;
        } float_lit;
    } u;
};

AstNode *ast_stmt_append(AstNode *head, AstNode *stmt);
AstNode *ast_program(AstNode *stmts);
AstNode *ast_decl(ValueType t, char *name, int line);
AstNode *ast_assign(char *name, AstNode *expr, int line);
AstNode *ast_if(AstNode *cond, AstNode *then_s, AstNode *else_s, int line);
AstNode *ast_while(AstNode *cond, AstNode *body, int line);
AstNode *ast_block(AstNode *stmts, int line);
AstNode *ast_expr_stmt(AstNode *e, int line);
AstNode *ast_return(AstNode *e, int line);
AstNode *ast_binop(AstBinOp op, AstNode *l, AstNode *r, int line);
AstNode *ast_unary(int op, AstNode *c, int line);
AstNode *ast_call(int is_log, AstNode *arg, int line);
AstNode *ast_var(char *name, int line);
AstNode *ast_int_lit(long long v, int line);
AstNode *ast_float_lit(double v, int line);

void ast_free(AstNode *n);

#endif
