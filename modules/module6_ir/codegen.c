#include "ast.h"
#include "codegen.h"
#include "tac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *gen_expr(TacProgram *p, AstNode *e);

static char *promote_if_needed(TacProgram *p, char *sub, ValueType need, ValueType have) {
    if (!sub)
        return NULL;
    if (need == TYPE_FLOAT && have == TYPE_INT) {
        char *t = tac_new_temp(p);
        tac_emit(p, TAC_OP_CAST_INT_TO_FLOAT, t, sub, NULL);
        free(sub);
        return t;
    }
    return sub;
}

/* Jump to label in `result` if arg1 rel arg2 (per op). */
static void emit_rel_jmp(TacProgram *p, TacOp rel, const char *label, const char *a1, const char *a2) {
    tac_emit(p, rel, label, a1, a2);
}

static char *gen_compare(TacProgram *p, AstBinOp o, char *l, char *r) {
    char *t = tac_new_temp(p);
    char *ltrue = tac_new_label(p);
    char *lend = tac_new_label(p);
    TacOp top = TAC_OP_IF_EQ;
    switch (o) {
    case AST_OP_EQ:
        top = TAC_OP_IF_EQ;
        break;
    case AST_OP_NE:
        top = TAC_OP_IF_NE;
        break;
    case AST_OP_LT:
        top = TAC_OP_IF_LT;
        break;
    case AST_OP_GT:
        top = TAC_OP_IF_GT;
        break;
    case AST_OP_LE:
        top = TAC_OP_IF_LE;
        break;
    case AST_OP_GE:
        top = TAC_OP_IF_GE;
        break;
    default:
        break;
    }
    emit_rel_jmp(p, top, ltrue, l, r);
    tac_emit(p, TAC_OP_COPY, t, "0", NULL);
    tac_emit(p, TAC_OP_GOTO, lend, NULL, NULL);
    tac_emit(p, TAC_OP_LABEL, ltrue, NULL, NULL);
    tac_emit(p, TAC_OP_COPY, t, "1", NULL);
    tac_emit(p, TAC_OP_LABEL, lend, NULL, NULL);
    free(ltrue);
    free(lend);
    free(l);
    free(r);
    return t;
}

static char *gen_expr(TacProgram *p, AstNode *e) {
    if (!e)
        return NULL;
    switch (e->kind) {
    case AST_INT_LIT: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%lld", (long long)e->u.int_lit.value);
        return strdup(buf);
    }
    case AST_FLOAT_LIT: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.6g", e->u.float_lit.value);
        return strdup(buf);
    }
    case AST_VAR:
        return strdup(e->u.var.name);
    case AST_ARR_INDEX: {
        char *idx = gen_expr(p, e->u.arr_index.index);
        char *t = tac_new_temp(p);
        /* TAC form:  t = name[idx]   (result, arg1=name, arg2=idx) */
        tac_emit(p, TAC_OP_ARR_LOAD, t, e->u.arr_index.name, idx);
        free(idx);
        return t;
    }
    case AST_UNARY: {
        char *c = gen_expr(p, e->u.unary.child);
        char *t = tac_new_temp(p);
        tac_emit(p, TAC_OP_NEG, t, c, NULL);
        free(c);
        return t;
    }
    case AST_CALL: {
        char *a = gen_expr(p, e->u.call.arg);
        char *t = tac_new_temp(p);
        if (e->u.call.is_log)
            tac_emit(p, TAC_OP_LOG, t, a, NULL);
        else
            tac_emit(p, TAC_OP_EXP, t, a, NULL);
        free(a);
        return t;
    }
    case AST_BINOP: {
        AstBinOp o = e->u.binop.op;
        if (o == AST_OP_EQ || o == AST_OP_NE || o == AST_OP_LT || o == AST_OP_GT || o == AST_OP_LE ||
            o == AST_OP_GE) {
            char *l = gen_expr(p, e->u.binop.left);
            char *r = gen_expr(p, e->u.binop.right);
            return gen_compare(p, o, l, r);
        }
        char *l = gen_expr(p, e->u.binop.left);
        char *r = gen_expr(p, e->u.binop.right);
        ValueType lt = e->u.binop.left->inferred_type;
        ValueType rt = e->u.binop.right->inferred_type;
        ValueType out = e->inferred_type;
        if (out == TYPE_FLOAT) {
            l = promote_if_needed(p, l, TYPE_FLOAT, lt);
            r = promote_if_needed(p, r, TYPE_FLOAT, rt);
        }
        char *t = tac_new_temp(p);
        TacOp op = TAC_OP_ADD;
        switch (o) {
        case AST_OP_ADD:
            op = TAC_OP_ADD;
            break;
        case AST_OP_SUB:
            op = TAC_OP_SUB;
            break;
        case AST_OP_MUL:
            op = TAC_OP_MUL;
            break;
        case AST_OP_DIV:
            op = TAC_OP_DIV;
            break;
        case AST_OP_POW:
            op = TAC_OP_POW;
            break;
        default:
            break;
        }
        tac_emit(p, op, t, l, r);
        free(l);
        free(r);
        return t;
    }
    default:
        return NULL;
    }
}

static void gen_stmt(TacProgram *p, AstNode *s) {
    if (!s)
        return;
    switch (s->kind) {
    case AST_ASSIGN: {
        char *rhs = gen_expr(p, s->u.assign.expr);
        tac_emit(p, TAC_OP_ASSIGN, s->u.assign.name, rhs, NULL);
        free(rhs);
        break;
    }
    case AST_ARR_ASSIGN: {
        char *idx = gen_expr(p, s->u.arr_assign.index);
        char *rhs = gen_expr(p, s->u.arr_assign.expr);
        /* TAC form:  name[idx] = rhs   (result=name, arg1=idx, arg2=rhs) */
        tac_emit(p, TAC_OP_ARR_STORE, s->u.arr_assign.name, idx, rhs);
        free(idx);
        free(rhs);
        break;
    }
    case AST_ARR_DECL:
        /* declarations are pure symbol-table actions */
        break;
    case AST_IF: {
        char *lelse = tac_new_label(p);
        char *lend = tac_new_label(p);
        char *t = gen_expr(p, s->u.iff.cond);
        emit_rel_jmp(p, TAC_OP_IF_EQ, lelse, t, "0");
        free(t);
        gen_stmt(p, s->u.iff.then_stmt);
        tac_emit(p, TAC_OP_GOTO, lend, NULL, NULL);
        tac_emit(p, TAC_OP_LABEL, lelse, NULL, NULL);
        if (s->u.iff.else_stmt)
            gen_stmt(p, s->u.iff.else_stmt);
        tac_emit(p, TAC_OP_LABEL, lend, NULL, NULL);
        free(lelse);
        free(lend);
        break;
    }
    case AST_WHILE: {
        char *lstart = tac_new_label(p);
        char *lend = tac_new_label(p);
        tac_emit(p, TAC_OP_LABEL, lstart, NULL, NULL);
        char *t = gen_expr(p, s->u.whloop.cond);
        emit_rel_jmp(p, TAC_OP_IF_EQ, lend, t, "0");
        free(t);
        gen_stmt(p, s->u.whloop.body);
        tac_emit(p, TAC_OP_GOTO, lstart, NULL, NULL);
        tac_emit(p, TAC_OP_LABEL, lend, NULL, NULL);
        free(lstart);
        free(lend);
        break;
    }
    case AST_BLOCK:
        for (AstNode *q = s->u.block.first; q; q = q->next)
            gen_stmt(p, q);
        break;
    case AST_DECL:
        /* declarations have no TAC unless initialized — omit */
        break;
    case AST_EXPR_STMT: {
        char *t = gen_expr(p, s->u.expr_stmt.expr);
        free(t);
        break;
    }
    case AST_RETURN: {
        char *t = gen_expr(p, s->u.ret.expr);
        if (t) {
            tac_emit(p, TAC_OP_COPY, "_ret", t, NULL);
            free(t);
        }
        break;
    }
    default:
        break;
    }
}

TacProgram *generate_tac(AstNode *prog) {
    TacProgram *p = tac_new();
    if (prog && prog->kind == AST_PROGRAM) {
        for (AstNode *s = prog->u.block.first; s; s = s->next)
            gen_stmt(p, s);
    }
    return p;
}
