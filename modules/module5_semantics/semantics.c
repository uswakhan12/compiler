#include "ast.h"
#include "semantics.h"
#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>

static int errors = 0;

static void sem_err(int line, const char *msg) {
    fprintf(stderr, "semantic error near line %d: %s\n", line, msg);
    errors++;
}

static ValueType check_expr(AstNode *e);

static ValueType check_expr(AstNode *e) {
    if (!e)
        return TYPE_UNKNOWN;
    switch (e->kind) {
    case AST_INT_LIT:
        e->inferred_type = TYPE_INT;
        return TYPE_INT;
    case AST_FLOAT_LIT:
        e->inferred_type = TYPE_FLOAT;
        return TYPE_FLOAT;
    case AST_VAR: {
        Symbol *s = symtab_lookup(e->u.var.name);
        if (!s) {
            sem_err(e->line, "use of undeclared variable");
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        }
        e->inferred_type = s->type;
        return s->type;
    }
    case AST_ARR_INDEX: {
        Symbol *s = symtab_lookup(e->u.arr_index.name);
        if (!s) {
            sem_err(e->line, "use of undeclared array");
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        }
        ValueType it = check_expr(e->u.arr_index.index);
        if (it != TYPE_INT)
            sem_err(e->line, "array index must be an integer");
        e->inferred_type = s->type;
        return s->type;
    }
    case AST_UNARY:
        if (e->u.unary.op == '-') {
            ValueType t = check_expr(e->u.unary.child);
            e->inferred_type = t;
            return t;
        }
        return TYPE_UNKNOWN;
    case AST_CALL: {
        check_expr(e->u.call.arg);
        e->inferred_type = TYPE_FLOAT;
        return TYPE_FLOAT;
    }
    case AST_BINOP: {
        ValueType a = check_expr(e->u.binop.left);
        ValueType b = check_expr(e->u.binop.right);
        AstBinOp o = e->u.binop.op;
        if (o == AST_OP_EQ || o == AST_OP_NE || o == AST_OP_LT || o == AST_OP_GT || o == AST_OP_LE ||
            o == AST_OP_GE) {
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        }
        ValueType out = (a == TYPE_FLOAT || b == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
        e->inferred_type = out;
        return out;
    }
    default:
        return TYPE_UNKNOWN;
    }
}

static void check_stmt(AstNode *s) {
    if (!s)
        return;
    switch (s->kind) {
    case AST_DECL:
        if (!symtab_insert(s->u.decl.name, s->u.decl.decl_type))
            sem_err(s->line, "redeclaration in same scope");
        break;
    case AST_ARR_DECL:
        if (!symtab_insert(s->u.arr_decl.name, s->u.arr_decl.elem_type))
            sem_err(s->line, "redeclaration in same scope");
        break;
    case AST_ARR_ASSIGN: {
        Symbol *sym = symtab_lookup(s->u.arr_assign.name);
        if (!sym)
            sem_err(s->line, "assignment to undeclared array");
        ValueType it = check_expr(s->u.arr_assign.index);
        if (it != TYPE_INT)
            sem_err(s->line, "array index must be an integer");
        ValueType rhs = check_expr(s->u.arr_assign.expr);
        if (sym && sym->type == TYPE_INT && rhs == TYPE_FLOAT)
            sem_err(s->line, "type mismatch: cannot store float into int array");
        break;
    }
    case AST_ASSIGN: {
        Symbol *sym = symtab_lookup(s->u.assign.name);
        if (!sym)
            sem_err(s->line, "assignment to undeclared variable");
        ValueType rhs = check_expr(s->u.assign.expr);
        if (sym && sym->type == TYPE_INT && rhs == TYPE_FLOAT)
            sem_err(s->line, "type mismatch: cannot assign float to int");
        break;
    }
    case AST_IF:
        check_expr(s->u.iff.cond);
        check_stmt(s->u.iff.then_stmt);
        if (s->u.iff.else_stmt)
            check_stmt(s->u.iff.else_stmt);
        break;
    case AST_WHILE:
        check_expr(s->u.whloop.cond);
        check_stmt(s->u.whloop.body);
        break;
    case AST_BLOCK:
        symtab_push_scope();
        for (AstNode *p = s->u.block.first; p; p = p->next)
            check_stmt(p);
        symtab_pop_scope();
        break;
    case AST_EXPR_STMT:
        check_expr(s->u.expr_stmt.expr);
        break;
    case AST_RETURN:
        check_expr(s->u.ret.expr);
        break;
    default:
        break;
    }
}

int semantic_check(AstNode *prog) {
    errors = 0;
    symtab_reset();
    symtab_push_scope();
    if (prog && prog->kind == AST_PROGRAM) {
        for (AstNode *p = prog->u.block.first; p; p = p->next)
            check_stmt(p);
    }
    symtab_pop_scope();
    return errors;
}
