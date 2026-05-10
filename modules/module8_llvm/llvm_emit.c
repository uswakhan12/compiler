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
    int is_float;     /* 0 = i32, 1 = double */
    int array_size;   /* 0 = scalar, >0 = array length */
    struct VarTy *next;
} VarTy;

/* Forward declarations — the temp-type inference pass below needs to
   call the literal helpers which are defined further down. */
static int is_number_literal(const char *s);
static int is_float_literal(const char *s);

static VarTy *g_vars;
static VarTy *g_temps;        /* Inferred types for tN temps */
static int g_decl_log = 0;    /* Whether we emit `declare double @log(double)` */
static int g_decl_exp = 0;
static int g_has_return = 0;  /* Whether the program has an explicit return */
/* A 1-deep queue is enough because our codegen emits `param a; call ...`
   adjacently and our log/exp lowering always has exactly one parameter. */
static char *g_pending_param = NULL;
static int   g_pending_param_isfloat = 0;

static void collect_var_types(AstNode *n) {
    if (!n)
        return;
    if (n->kind == AST_DECL) {
        VarTy *v = (VarTy *)calloc(1, sizeof(VarTy));
        v->name = strdup(n->u.decl.name);
        v->is_float = (n->u.decl.decl_type == TYPE_FLOAT);
        v->array_size = 0;
        v->next = g_vars;
        g_vars = v;
        return;
    }
    if (n->kind == AST_ARR_DECL) {
        VarTy *v = (VarTy *)calloc(1, sizeof(VarTy));
        v->name = strdup(n->u.arr_decl.name);
        v->is_float = (n->u.arr_decl.elem_type == TYPE_FLOAT);
        v->array_size = (int)n->u.arr_decl.size;
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

static VarTy *lookup_var(const char *name) {
    if (!name)
        return NULL;
    for (VarTy *v = g_vars; v; v = v->next)
        if (strcmp(v->name, name) == 0)
            return v;
    return NULL;
}

static int temp_is_float(const char *t) {
    if (!t)
        return 0;
    for (VarTy *v = g_temps; v; v = v->next)
        if (strcmp(v->name, t) == 0)
            return v->is_float;
    return 0;
}

static void set_temp_float(const char *t, int isf) {
    for (VarTy *v = g_temps; v; v = v->next)
        if (strcmp(v->name, t) == 0) {
            if (isf)
                v->is_float = 1;
            return;
        }
    VarTy *v = (VarTy *)calloc(1, sizeof(VarTy));
    v->name = strdup(t);
    v->is_float = isf;
    v->next = g_temps;
    g_temps = v;
}

/* Pre-pass: infer i32-vs-double for every temp, with fixed-point
   iteration because a temp's type may depend on another temp's type. */
static void infer_temp_types(TacProgram *tac) {
    /* First insert every temp with is_float=0 (default). */
    for (TacInst *in = tac->head; in; in = in->next) {
        if (in->result && in->result[0] == 't')
            set_temp_float(in->result, 0);
    }
    /* Helper: operand is float if it's a float literal, a float source
       variable, or a temp already inferred as float. */
#define OP_FLOAT(o) ((o) ? (is_number_literal(o) ? is_float_literal(o)               \
                                              : (lookup_is_float(o) || temp_is_float(o)))  \
                       : 0)
    int changed = 1;
    while (changed) {
        changed = 0;
        for (TacInst *in = tac->head; in; in = in->next) {
            if (!in->result || in->result[0] != 't')
                continue;
            int isf = 0;
            switch (in->op) {
            case TAC_OP_CALL:
                /* `t = call log/exp, ...` produces double. */
                if (in->arg1 && (strcmp(in->arg1, "log") == 0 || strcmp(in->arg1, "exp") == 0))
                    isf = 1;
                break;
            case TAC_OP_LOG:
            case TAC_OP_EXP:
            case TAC_OP_CAST_INT_TO_FLOAT:
                isf = 1;
                break;
            case TAC_OP_ARR_LOAD: {
                VarTy *arr = lookup_var(in->arg1);
                isf = arr ? arr->is_float : 0;
                break;
            }
            case TAC_OP_ADD:
            case TAC_OP_SUB:
            case TAC_OP_MUL:
            case TAC_OP_DIV:
            case TAC_OP_POW:
                isf = OP_FLOAT(in->arg1) || OP_FLOAT(in->arg2);
                break;
            case TAC_OP_NEG:
                isf = OP_FLOAT(in->arg1);
                break;
            case TAC_OP_COPY:
            case TAC_OP_ASSIGN:
                isf = OP_FLOAT(in->arg1);
                break;
            default:
                break;
            }
            if (isf && !temp_is_float(in->result)) {
                set_temp_float(in->result, 1);
                changed = 1;
            }
        }
    }
#undef OP_FLOAT
}

/* Defined here (forward-declared above for the temp-typing pass). */
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
    if (lookup_is_float(o))
        return 1;
    return temp_is_float(o);
}

static const char *llvm_type_of(const char *name) {
    return operand_is_float(name) ? "double" : "i32";
}

static void scan_for_decls(TacProgram *tac) {
    for (TacInst *in = tac->head; in; in = in->next) {
        if (in->op == TAC_OP_CALL && in->arg1) {
            if (strcmp(in->arg1, "log") == 0)
                g_decl_log = 1;
            else if (strcmp(in->arg1, "exp") == 0)
                g_decl_exp = 1;
        }
        if (in->op == TAC_OP_RETURN)
            g_has_return = 1;
    }
}

static void emit_prelude(FILE *out) {
    fprintf(out, "; ModuleID = 'minicc-output'\n");
    fprintf(out, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
    fprintf(out, "@.fmt_int = private constant [4 x i8] c\"%%d\\0A\\00\"\n");
    fprintf(out, "@.fmt_flt = private constant [4 x i8] c\"%%f\\0A\\00\"\n");
    fprintf(out, "declare i32 @printf(i8*, ...)\n");
    if (g_decl_log)
        fprintf(out, "declare double @log(double)\n");
    if (g_decl_exp)
        fprintf(out, "declare double @exp(double)\n");
    fprintf(out, "\ndefine i32 @main() {\n");
    fprintf(out, "entry:\n");
}

static void emit_alloca_for_locals(FILE *out, TacProgram *tac) {
    for (VarTy *v = g_vars; v; v = v->next) {
        const char *elt = v->is_float ? "double" : "i32";
        if (v->array_size > 0) {
            fprintf(out, "  %%%s = alloca [%d x %s]\n", v->name, v->array_size, elt);
        } else {
            fprintf(out, "  %%%s = alloca %s\n", v->name, elt);
            fprintf(out, "  store %s %s, %s* %%%s\n", elt, v->is_float ? "0.000000e+00" : "0",
                    elt, v->name);
        }
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

/* result = arr[idx] */
static void emit_arr_load(FILE *out, TacInst *in) {
    VarTy *arr = lookup_var(in->arg1);
    if (!arr) {
        fprintf(out, "  ; (array load: unknown array %s)\n", in->arg1 ? in->arg1 : "?");
        return;
    }
    const char *elt = arr->is_float ? "double" : "i32";
    char *idx = emit_load(out, in->arg2, 0);
    char *gep = fresh_ssa("gep");
    fprintf(out, "  %s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %s\n", gep,
            arr->array_size, elt, arr->array_size, elt, in->arg1, idx);
    char *ld = fresh_ssa("ald");
    fprintf(out, "  %s = load %s, %s* %s\n", ld, elt, elt, gep);
    emit_store(out, in->result, ld, arr->is_float);
    free(idx);
    free(gep);
    free(ld);
}

/* arr[idx] = rhs  (result holds arr name, arg1=idx, arg2=rhs) */
static void emit_arr_store(FILE *out, TacInst *in) {
    VarTy *arr = lookup_var(in->result);
    if (!arr) {
        fprintf(out, "  ; (array store: unknown array %s)\n", in->result ? in->result : "?");
        return;
    }
    const char *elt = arr->is_float ? "double" : "i32";
    char *idx = emit_load(out, in->arg1, 0);
    char *val = emit_load(out, in->arg2, arr->is_float);
    char *gep = fresh_ssa("gep");
    fprintf(out, "  %s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %s\n", gep,
            arr->array_size, elt, arr->array_size, elt, in->result, idx);
    fprintf(out, "  store %s %s, %s* %s\n", elt, val, elt, gep);
    free(idx);
    free(val);
    free(gep);
}

void emit_llvm_ir(FILE *out, AstNode *program, TacProgram *tac) {
    g_vars = NULL;
    g_temps = NULL;
    g_ssa_counter = 0;
    g_decl_log = g_decl_exp = g_has_return = 0;
    g_pending_param = NULL;
    g_pending_param_isfloat = 0;
    collect_var_types(program);
    scan_for_decls(tac);
    infer_temp_types(tac);
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
        case TAC_OP_ARR_LOAD:
            emit_arr_load(out, in);
            break;
        case TAC_OP_ARR_STORE:
            emit_arr_store(out, in);
            break;
        case TAC_OP_PARAM: {
            /* Queue the parameter; load now so the value is fresh. */
            int is_f = operand_is_float(in->arg1);
            char *v = emit_load(out, in->arg1, is_f);
            free(g_pending_param);
            g_pending_param = v;
            g_pending_param_isfloat = is_f;
            break;
        }
        case TAC_OP_CALL: {
            /* `result = call <fname>, <argc>` — we currently support the
               math built-ins log and exp (1 arg, double). */
            const char *fname = in->arg1 ? in->arg1 : "?";
            int is_math = (strcmp(fname, "log") == 0 || strcmp(fname, "exp") == 0);
            if (is_math) {
                /* Promote the queued param to double if needed. */
                char *arg = g_pending_param ? strdup(g_pending_param) : strdup("0.000000e+00");
                if (g_pending_param && !g_pending_param_isfloat) {
                    char *s = fresh_ssa("argd");
                    fprintf(out, "  %s = sitofp i32 %s to double\n", s, arg);
                    free(arg);
                    arg = s;
                }
                char *r = fresh_ssa("ret");
                fprintf(out, "  %s = call double @%s(double %s)\n", r, fname, arg);
                if (in->result)
                    emit_store(out, in->result, r, 1);
                free(arg);
                free(r);
            } else {
                fprintf(out, "  ; (unsupported call to %s)\n", fname);
            }
            free(g_pending_param);
            g_pending_param = NULL;
            g_pending_param_isfloat = 0;
            break;
        }
        case TAC_OP_RETURN: {
            /* `return x` — the runtime ABI for our `main()` is `i32`,
               so coerce double → i32 when necessary. */
            int is_f = operand_is_float(in->arg1);
            char *v = emit_load(out, in->arg1, is_f);
            if (is_f) {
                char *s = fresh_ssa("rcast");
                fprintf(out, "  %s = fptosi double %s to i32\n", s, v);
                free(v);
                v = s;
            }
            fprintf(out, "  ret i32 %s\n", v);
            /* Place a fresh anchor block so any following code stays
               well-formed LLVM (every block must have one terminator). */
            fprintf(out, "post.%d:\n", g_ssa_counter++);
            free(v);
            break;
        }
        default:
            fprintf(out, "  ; (skipping unsupported TAC op: id=%d)\n", in->id);
            break;
        }
    }

    /* If the program had no explicit `return`, terminate `main` with
       `ret i32 0`. Otherwise the explicit return already terminated. */
    if (!g_has_return)
        fprintf(out, "  ret i32 0\n");
    else
        fprintf(out, "  ret i32 0     ; fallback terminator\n");
    fprintf(out, "}\n");

    free(g_pending_param);
    g_pending_param = NULL;
    while (g_vars) {
        VarTy *nx = g_vars->next;
        free(g_vars->name);
        free(g_vars);
        g_vars = nx;
    }
    while (g_temps) {
        VarTy *nx = g_temps->next;
        free(g_temps->name);
        free(g_temps);
        g_temps = nx;
    }
}
