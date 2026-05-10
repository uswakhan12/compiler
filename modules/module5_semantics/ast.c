#include "ast.h"
#include <stdlib.h>
#include <string.h>

static AstNode *mk(AstKind k, int line) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    n->kind = k;
    n->line = line;
    n->inferred_type = TYPE_UNKNOWN;
    n->next = NULL;
    return n;
}

AstNode *ast_stmt_append(AstNode *head, AstNode *stmt) {
    if (!stmt)
        return head;
    if (!head)
        return stmt;
    AstNode *p = head;
    while (p->next)
        p = p->next;
    p->next = stmt;
    return head;
}

AstNode *ast_program(AstNode *stmts) {
    AstNode *n = mk(AST_PROGRAM, 1);
    n->u.block.first = stmts;
    return n;
}

AstNode *ast_decl(ValueType t, char *name, int line) {
    AstNode *n = mk(AST_DECL, line);
    n->u.decl.decl_type = t;
    n->u.decl.name = name;
    return n;
}

AstNode *ast_assign(char *name, AstNode *expr, int line) {
    AstNode *n = mk(AST_ASSIGN, line);
    n->u.assign.name = name;
    n->u.assign.expr = expr;
    return n;
}

AstNode *ast_arr_decl(ValueType elem, char *name, long long size, int line) {
    AstNode *n = mk(AST_ARR_DECL, line);
    n->u.arr_decl.elem_type = elem;
    n->u.arr_decl.name = name;
    n->u.arr_decl.size = size;
    return n;
}

AstNode *ast_arr_assign(char *name, AstNode *index, AstNode *expr, int line) {
    AstNode *n = mk(AST_ARR_ASSIGN, line);
    n->u.arr_assign.name = name;
    n->u.arr_assign.index = index;
    n->u.arr_assign.expr = expr;
    return n;
}

AstNode *ast_arr_index(char *name, AstNode *index, int line) {
    AstNode *n = mk(AST_ARR_INDEX, line);
    n->u.arr_index.name = name;
    n->u.arr_index.index = index;
    return n;
}

AstNode *ast_if(AstNode *cond, AstNode *then_s, AstNode *else_s, int line) {
    AstNode *n = mk(AST_IF, line);
    n->u.iff.cond = cond;
    n->u.iff.then_stmt = then_s;
    n->u.iff.else_stmt = else_s;
    return n;
}

AstNode *ast_while(AstNode *cond, AstNode *body, int line) {
    AstNode *n = mk(AST_WHILE, line);
    n->u.whloop.cond = cond;
    n->u.whloop.body = body;
    return n;
}

AstNode *ast_block(AstNode *stmts, int line) {
    AstNode *n = mk(AST_BLOCK, line);
    n->u.block.first = stmts;
    return n;
}

AstNode *ast_expr_stmt(AstNode *e, int line) {
    AstNode *n = mk(AST_EXPR_STMT, line);
    n->u.expr_stmt.expr = e;
    return n;
}

AstNode *ast_return(AstNode *e, int line) {
    AstNode *n = mk(AST_RETURN, line);
    n->u.ret.expr = e;
    return n;
}

AstNode *ast_binop(AstBinOp op, AstNode *l, AstNode *r, int line) {
    AstNode *n = mk(AST_BINOP, line);
    n->u.binop.op = op;
    n->u.binop.left = l;
    n->u.binop.right = r;
    return n;
}

AstNode *ast_unary(int op, AstNode *c, int line) {
    AstNode *n = mk(AST_UNARY, line);
    n->u.unary.op = op;
    n->u.unary.child = c;
    return n;
}

AstNode *ast_call(int is_log, AstNode *arg, int line) {
    AstNode *n = mk(AST_CALL, line);
    n->u.call.is_log = is_log;
    n->u.call.arg = arg;
    return n;
}

AstNode *ast_var(char *name, int line) {
    AstNode *n = mk(AST_VAR, line);
    n->u.var.name = name;
    return n;
}

AstNode *ast_int_lit(long long v, int line) {
    AstNode *n = mk(AST_INT_LIT, line);
    n->u.int_lit.value = v;
    n->inferred_type = TYPE_INT;
    return n;
}

AstNode *ast_float_lit(double v, int line) {
    AstNode *n = mk(AST_FLOAT_LIT, line);
    n->u.float_lit.value = v;
    n->inferred_type = TYPE_FLOAT;
    return n;
}

static void free_expr_tree(AstNode *n) {
    if (!n)
        return;
    switch (n->kind) {
    case AST_DECL:
        free(n->u.decl.name);
        break;
    case AST_ARR_DECL:
        free(n->u.arr_decl.name);
        break;
    case AST_ARR_ASSIGN:
        free(n->u.arr_assign.name);
        free_expr_tree(n->u.arr_assign.index);
        free_expr_tree(n->u.arr_assign.expr);
        break;
    case AST_ARR_INDEX:
        free(n->u.arr_index.name);
        free_expr_tree(n->u.arr_index.index);
        break;
    case AST_ASSIGN:
        free(n->u.assign.name);
        free_expr_tree(n->u.assign.expr);
        break;
    case AST_IF:
        free_expr_tree(n->u.iff.cond);
        ast_free(n->u.iff.then_stmt);
        ast_free(n->u.iff.else_stmt);
        break;
    case AST_WHILE:
        free_expr_tree(n->u.whloop.cond);
        ast_free(n->u.whloop.body);
        break;
    case AST_BLOCK:
        ast_free(n->u.block.first);
        break;
    case AST_EXPR_STMT:
        free_expr_tree(n->u.expr_stmt.expr);
        break;
    case AST_RETURN:
        free_expr_tree(n->u.ret.expr);
        break;
    case AST_BINOP:
        free_expr_tree(n->u.binop.left);
        free_expr_tree(n->u.binop.right);
        break;
    case AST_UNARY:
        free_expr_tree(n->u.unary.child);
        break;
    case AST_CALL:
        free_expr_tree(n->u.call.arg);
        break;
    case AST_VAR:
        free(n->u.var.name);
        break;
    default:
        break;
    }
}

void ast_free(AstNode *n) {
    if (!n)
        return;
    if (n->kind == AST_PROGRAM) {
        AstNode *s = n->u.block.first;
        free(n);
        ast_free(s);
        return;
    }
    while (n) {
        AstNode *nx = n->next;
        n->next = NULL;
        free_expr_tree(n);
        free(n);
        n = nx;
    }
}
