/*
 * Module 8 — LLVM IR backend.
 *
 * Translates our Three-Address Code (TAC) into textual LLVM IR.
 * The result is a self-contained `.ll` file that Clang can compile
 * to a native executable, completing the full pipeline:
 *
 *    source.mini → tokens → AST → TAC → optimised TAC → LLVM IR → exe
 *
 * Design notes
 *   * Every source variable is given a stack slot (`alloca i32`/`double`).
 *   * Every temp `tN` is also stack-allocated for simplicity (no SSA).
 *   * Loads/stores wrap every TAC operand and every TAC destination.
 *   * Integer ops use `add/sub/mul/sdiv`; float ops use `f*`.
 *   * Conditional jumps lower to icmp + br with the target labels we
 *     already emitted in TAC.
 *   * For demonstration we treat all variables as i32 by default; if
 *     the source AST declared `float x;`, the slot becomes `double`
 *     and we promote operands as required.
 */

#include "llvm_emit.h"
#include "ast.h"
#include "tac.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VarTy {
    char *name;
    int is_float; /* 0 = i32, 1 = double */
    struct VarTy *next;
} VarTy;

static VarTy *g_vars;

static void collect_var_types(AstNode *n) {
    if (!n)
        return;
    if (n->kind == AST_DECL) {
        VarTy *v = (VarTy *)calloc(1, sizeof(VarTy));
        v->name = strdup(n->u.decl.name);
        v->is_float = (n->u.decl.decl_type == TYPE_FLOAT);
        v->next = g_vars;
        g_vars = v;
        return;
    }
    if (n->kind == AST_PROGRAM || n->kind == AST_BLOCK) {
        for (AstNode *q = n->u.block.first; q; q = q->next)
            collect_var_types(q);
        return;
    }
    if (n->kind == AST_IF) {
        collect_var_types(n->u.iff.then_stmt);
        collect_var_types(n->u.iff.else_stmt);
        return;
    }
    if (n->kind == AST_WHILE) {
        collect_var_types(n->u.whloop.body);
        return;
    }
}

static int lookup_is_float(const char *name) {
    if (!name)
        return 0;
    for (VarTy *v = g_vars; v; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v->is_float;
    return 0;
}

static int is_number_literal(const char *s) {
    if (!s || !*s)
        return 0;
    char *end = NULL;
    strtod(s, &end);
    return end && *end == '\0';
}

static int is_float_literal(const char *s) {
    if (!is_number_literal(s))
        return 0;
    for (const char *p = s; *p; p++)
        if (*p == '.' || *p == 'e' || *p == 'E')
            return 1;
    return 0;
}

/* Operand may be a constant, a source variable, or a temp `tN`. */
static int operand_is_float(const char *o) {
    if (!o)
        return 0;
    if (is_number_literal(o))
        return is_float_literal(o);
    return lookup_is_float(o);
}

static const char *llvm_type_of(const char *name) {
    return operand_is_float(name) ? "double" : "i32";
}

static void emit_prelude(FILE *out) {
    fprintf(out, "; ModuleID = 'minicc-output'\n");
    fprintf(out, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
    fprintf(out, "@.fmt_int = private constant [4 x i8] c\"%%d\\0A\\00\"\n");
    fprintf(out, "@.fmt_flt = private constant [4 x i8] c\"%%f\\0A\\00\"\n");
    fprintf(out, "declare i32 @printf(i8*, ...)\n\n");
    fprintf(out, "define i32 @main() {\n");
    fprintf(out, "entry:\n");
}

static void emit_alloca_for_locals(FILE *out, TacProgram *tac) {
    for (VarTy *v = g_vars; v; v = v->next) {
        fprintf(out, "  %%%s = alloca %s\n", v->name, v->is_float ? "double" : "i32");
        fprintf(out, "  store %s %s, %s* %%%s\n", v->is_float ? "double" : "i32",
                v->is_float ? "0.000000e+00" : "0", v->is_float ? "double" : "i32", v->name);
    }
    /* Temps */
    for (TacInst *in = tac->head; in; in = in->next) {
        if (!in->result)
            continue;
        if (in->result[0] != 't')
            continue;
        if (in->op == TAC_OP_LABEL)
            continue;
        const char *ty = llvm_type_of(in->result);
        /* Avoid duplicate alloca for the same temp. */
        int seen = 0;
        for (TacInst *q = tac->head; q != in; q = q->next) {
            if (q->result && q->result[0] == 't' && strcmp(q->result, in->result) == 0) {
                seen = 1;
                break;
            }
        }
        if (!seen)
            fprintf(out, "  %%%s = alloca %s\n", in->result, ty);
    }
}

static int g_ssa_counter;

static char *fresh_ssa(const char *hint) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%%%s.%d", hint, g_ssa_counter++);
    return strdup(buf);
}

/* Emit a load of operand `o` and return the SSA name holding the value.
   For numeric literals, returns the literal text (no load). */
static char *emit_load(FILE *out, const char *o, int want_float) {
    if (is_number_literal(o)) {
        if (want_float && !is_float_literal(o)) {
            char *s = fresh_ssa("flit");
            fprintf(out, "  %s = sitofp i32 %s to double\n", s, o);
            return s;
        }
        return strdup(o);
    }
    int is_f = lookup_is_float(o) || (o[0] == 't' && operand_is_float(o));
    char *s = fresh_ssa("ld");
    fprintf(out, "  %s = load %s, %s* %%%s\n", s, is_f ? "double" : "i32",
            is_f ? "double" : "i32", o);
    if (want_float && !is_f) {
        char *s2 = fresh_ssa("p");
        fprintf(out, "  %s = sitofp i32 %s to double\n", s2, s);
        free(s);
        return s2;
    }
    return s;
}

static void emit_store(FILE *out, const char *dest, const char *ssa_val, int val_is_float) {
    int dest_is_float = lookup_is_float(dest) || (dest && dest[0] == 't' && operand_is_float(dest));
    char *coerced = strdup(ssa_val);
    if (dest_is_float && !val_is_float) {
        char *s = fresh_ssa("p");
        fprintf(out, "  %s = sitofp i32 %s to double\n", s, coerced);
        free(coerced);
        coerced = s;
    } else if (!dest_is_float && val_is_float) {
        char *s = fresh_ssa("p");
        fprintf(out, "  %s = fptosi double %s to i32\n", s, coerced);
        free(coerced);
        coerced = s;
    }
    fprintf(out, "  store %s %s, %s* %%%s\n", dest_is_float ? "double" : "i32", coerced,
            dest_is_float ? "double" : "i32", dest);
    free(coerced);
}

static const char *iop(TacOp op) {
    switch (op) {
    case TAC_OP_ADD:
        return "add";
    case TAC_OP_SUB:
        return "sub";
    case TAC_OP_MUL:
        return "mul";
    case TAC_OP_DIV:
        return "sdiv";
    default:
        return "add";
    }
}

static const char *fop(TacOp op) {
    switch (op) {
    case TAC_OP_ADD:
        return "fadd";
    case TAC_OP_SUB:
        return "fsub";
    case TAC_OP_MUL:
        return "fmul";
    case TAC_OP_DIV:
        return "fdiv";
    default:
        return "fadd";
    }
}

static void emit_binop(FILE *out, TacInst *in) {
    int want_float =
        operand_is_float(in->arg1) || operand_is_float(in->arg2) || operand_is_float(in->result);
    char *l = emit_load(out, in->arg1, want_float);
    char *r = emit_load(out, in->arg2, want_float);
    char *res = fresh_ssa("v");
    fprintf(out, "  %s = %s %s %s, %s\n", res, want_float ? fop(in->op) : iop(in->op),
            want_float ? "double" : "i32", l, r);
    emit_store(out, in->result, res, want_float);
    free(l);
    free(r);
    free(res);
}

static const char *icmp_pred(TacOp op) {
    switch (op) {
    case TAC_OP_IF_EQ:
        return "eq";
    case TAC_OP_IF_NE:
        return "ne";
    case TAC_OP_IF_LT:
        return "slt";
    case TAC_OP_IF_GT:
        return "sgt";
    case TAC_OP_IF_LE:
        return "sle";
    case TAC_OP_IF_GE:
        return "sge";
    default:
        return "eq";
    }
}

static void emit_branch(FILE *out, TacInst *in) {
    int want_float = operand_is_float(in->arg1) || operand_is_float(in->arg2);
    char *l = emit_load(out, in->arg1, want_float);
    char *r = emit_load(out, in->arg2, want_float);
    char *cond = fresh_ssa("c");
    fprintf(out, "  %s = icmp %s i32 %s, %s\n", cond, icmp_pred(in->op), l, r);
    /* Fallthrough label is constructed by inserting a synthetic label after the branch. */
    fprintf(out, "  br i1 %s, label %%%s, label %%cont.%d\n", cond, in->result, g_ssa_counter);
    fprintf(out, "cont.%d:\n", g_ssa_counter);
    g_ssa_counter++;
    free(l);
    free(r);
    free(cond);
}

static void emit_copy(FILE *out, TacInst *in) {
    int want_float = operand_is_float(in->arg1) || operand_is_float(in->result);
    char *v = emit_load(out, in->arg1, want_float);
    emit_store(out, in->result, v, want_float);
    free(v);
}

static void emit_neg(FILE *out, TacInst *in) {
    int want_float = operand_is_float(in->arg1) || operand_is_float(in->result);
    char *v = emit_load(out, in->arg1, want_float);
    char *res = fresh_ssa("n");
    if (want_float)
        fprintf(out, "  %s = fsub double 0.000000e+00, %s\n", res, v);
    else
        fprintf(out, "  %s = sub i32 0, %s\n", res, v);
    emit_store(out, in->result, res, want_float);
    free(v);
    free(res);
}

static void emit_cast(FILE *out, TacInst *in) {
    char *v = emit_load(out, in->arg1, 0);
    char *res = fresh_ssa("p");
    fprintf(out, "  %s = sitofp i32 %s to double\n", res, v);
    emit_store(out, in->result, res, 1);
    free(v);
    free(res);
}

void emit_llvm_ir(FILE *out, AstNode *program, TacProgram *tac) {
    g_vars = NULL;
    g_ssa_counter = 0;
    collect_var_types(program);
    emit_prelude(out);
    emit_alloca_for_locals(out, tac);

    for (TacInst *in = tac->head; in; in = in->next) {
        switch (in->op) {
        case TAC_OP_LABEL:
            /* Make sure the basic block before us is terminated. */
            fprintf(out, "  br label %%%s\n", in->result);
            fprintf(out, "%s:\n", in->result);
            break;
        case TAC_OP_GOTO:
            fprintf(out, "  br label %%%s\n", in->result);
            fprintf(out, "post.%d:\n", g_ssa_counter++);
            break;
        case TAC_OP_IF_EQ:
        case TAC_OP_IF_NE:
        case TAC_OP_IF_LT:
        case TAC_OP_IF_GT:
        case TAC_OP_IF_LE:
        case TAC_OP_IF_GE:
            emit_branch(out, in);
            break;
        case TAC_OP_ADD:
        case TAC_OP_SUB:
        case TAC_OP_MUL:
        case TAC_OP_DIV:
            emit_binop(out, in);
            break;
        case TAC_OP_NEG:
            emit_neg(out, in);
            break;
        case TAC_OP_ASSIGN:
        case TAC_OP_COPY:
            emit_copy(out, in);
            break;
        case TAC_OP_CAST_INT_TO_FLOAT:
            emit_cast(out, in);
            break;
        default:
            fprintf(out, "  ; (skipping unsupported TAC op: id=%d)\n", in->id);
            break;
        }
    }

    fprintf(out, "  ret i32 0\n");
    fprintf(out, "}\n");

    while (g_vars) {
        VarTy *nx = g_vars->next;
        free(g_vars->name);
        free(g_vars);
        g_vars = nx;
    }
}
