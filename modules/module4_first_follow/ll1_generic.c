/*
 * Module 4 — generic FIRST / FOLLOW / LL(1) parsing-table builder.
 *
 * Reads a context-free grammar from an input file (or stdin) and
 * computes:
 *   1. FIRST sets for every non-terminal
 *   2. FOLLOW sets for every non-terminal
 *   3. The LL(1) parsing table M[A,a] = production index
 *
 * Grammar file format:
 *   LHS -> rhs1 rhs2 ... | rhs1 ... | epsilon
 *   ...
 *   # lines starting with '#' or blank lines are ignored
 *
 * Symbol rules:
 *   * Non-terminals: tokens that appear on the LHS of any rule.
 *   * Terminals:     everything else (e.g. id, num, '+', '(', ')', ...).
 *   * Epsilon:       the literal token  epsilon  or  eps  or  ε.
 *   * End marker:    '$' (always added to FOLLOW(start-symbol)).
 *
 * Build:    gcc -std=c11 -O2 -o ll1_generic ll1_generic.c
 * Run:      ./ll1_generic grammars/expr.txt
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 256
#define MAX_RHS 16
#define MAX_PRODS 256

static char *g_symbols[MAX_SYMBOLS];
static int g_nsyms = 0;
static int g_is_nt[MAX_SYMBOLS];

typedef struct {
    int lhs;
    int rhs[MAX_RHS];
    int len;
} Prod;

static Prod g_prods[MAX_PRODS];
static int g_nprods = 0;
static int g_start = -1;

static int g_nullable[MAX_SYMBOLS];
static int g_first[MAX_SYMBOLS][MAX_SYMBOLS];
static int g_nfirst[MAX_SYMBOLS];
static int g_follow[MAX_SYMBOLS][MAX_SYMBOLS];
static int g_nfollow[MAX_SYMBOLS];
static int g_table[MAX_SYMBOLS][MAX_SYMBOLS];

static int sym_intern(const char *name) {
    if (strcmp(name, "epsilon") == 0 || strcmp(name, "eps") == 0 ||
        strcmp(name, "\xce\xb5" /* ε */) == 0)
        return -1; /* epsilon sentinel */
    for (int i = 0; i < g_nsyms; i++)
        if (strcmp(g_symbols[i], name) == 0)
            return i;
    if (g_nsyms >= MAX_SYMBOLS) {
        fprintf(stderr, "Too many symbols (max %d)\n", MAX_SYMBOLS);
        exit(1);
    }
    g_symbols[g_nsyms] = strdup(name);
    return g_nsyms++;
}

static void add_unique(int *arr, int *n, int v) {
    for (int i = 0; i < *n; i++)
        if (arr[i] == v)
            return;
    arr[(*n)++] = v;
}

static char *strip_inplace(char *s) {
    while (*s && isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)*(e - 1)))
        --e;
    *e = '\0';
    return s;
}

static void add_production(int lhs, int *rhs, int len) {
    if (g_nprods >= MAX_PRODS) {
        fprintf(stderr, "Too many productions (max %d)\n", MAX_PRODS);
        exit(1);
    }
    g_prods[g_nprods].lhs = lhs;
    g_prods[g_nprods].len = len;
    for (int i = 0; i < len; i++)
        g_prods[g_nprods].rhs[i] = rhs[i];
    g_nprods++;
    if (g_start < 0)
        g_start = lhs;
}

static void parse_line(char *line) {
    char *s = strip_inplace(line);
    if (!*s || *s == '#')
        return;
    char *arrow = strstr(s, "->");
    if (!arrow) {
        fprintf(stderr, "skip (no arrow): %s\n", s);
        return;
    }
    *arrow = '\0';
    char *lhs_str = strip_inplace(s);
    char *body = strip_inplace(arrow + 2);
    int lhs = sym_intern(lhs_str);
    g_is_nt[lhs] = 1;

    /* Body may contain `|` separating alternatives. */
    char *alt = body;
    while (alt) {
        char *bar = strchr(alt, '|');
        if (bar)
            *bar = '\0';
        char *a = strip_inplace(alt);
        int rhs[MAX_RHS];
        int n = 0;
        if (*a) {
            char *tok = strtok(a, " \t");
            while (tok) {
                int t = sym_intern(tok);
                if (t < 0) {
                    /* epsilon → empty rhs */
                } else {
                    rhs[n++] = t;
                }
                tok = strtok(NULL, " \t");
            }
        }
        add_production(lhs, rhs, n);
        if (!bar)
            break;
        alt = bar + 1;
    }
}

static int is_terminal(int s) { return !g_is_nt[s]; }

static int first_of_seq(int *rhs, int len, int *out, int *nout) {
    /* returns 1 if sequence is nullable */
    *nout = 0;
    int nullable = 1;
    for (int i = 0; i < len; i++) {
        int s = rhs[i];
        if (is_terminal(s)) {
            add_unique(out, nout, s);
            return 0;
        }
        for (int j = 0; j < g_nfirst[s]; j++)
            add_unique(out, nout, g_first[s][j]);
        if (!g_nullable[s])
            return 0;
    }
    return nullable;
}

static void compute_first(void) {
    memset(g_nfirst, 0, sizeof(g_nfirst));
    memset(g_nullable, 0, sizeof(g_nullable));
    for (int s = 0; s < g_nsyms; s++)
        if (is_terminal(s)) {
            g_first[s][0] = s;
            g_nfirst[s] = 1;
        }
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int p = 0; p < g_nprods; p++) {
            int lhs = g_prods[p].lhs;
            if (g_prods[p].len == 0) {
                if (!g_nullable[lhs]) {
                    g_nullable[lhs] = 1;
                    changed = 1;
                }
                continue;
            }
            int buf[MAX_SYMBOLS];
            int nb = 0;
            int nullseq = first_of_seq(g_prods[p].rhs, g_prods[p].len, buf, &nb);
            int old = g_nfirst[lhs];
            for (int i = 0; i < nb; i++)
                add_unique(g_first[lhs], &g_nfirst[lhs], buf[i]);
            if (nullseq && !g_nullable[lhs]) {
                g_nullable[lhs] = 1;
                changed = 1;
            }
            if (g_nfirst[lhs] != old)
                changed = 1;
        }
    }
}

static int g_dollar = -1;

static void compute_follow(void) {
    memset(g_nfollow, 0, sizeof(g_nfollow));
    g_dollar = sym_intern("$");
    add_unique(g_follow[g_start], &g_nfollow[g_start], g_dollar);
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int p = 0; p < g_nprods; p++) {
            int lhs = g_prods[p].lhs;
            int *R = g_prods[p].rhs;
            int L = g_prods[p].len;
            for (int i = 0; i < L; i++) {
                int B = R[i];
                if (is_terminal(B))
                    continue;
                int tail[MAX_SYMBOLS];
                int nt = 0;
                int nullseq = first_of_seq(R + i + 1, L - i - 1, tail, &nt);
                int old = g_nfollow[B];
                for (int j = 0; j < nt; j++)
                    add_unique(g_follow[B], &g_nfollow[B], tail[j]);
                if (nullseq || i == L - 1) {
                    for (int j = 0; j < g_nfollow[lhs]; j++)
                        add_unique(g_follow[B], &g_nfollow[B], g_follow[lhs][j]);
                }
                if (g_nfollow[B] != old)
                    changed = 1;
            }
        }
    }
}

static int g_conflicts = 0;

static void build_table(void) {
    for (int a = 0; a < MAX_SYMBOLS; a++)
        for (int t = 0; t < MAX_SYMBOLS; t++)
            g_table[a][t] = -1;
    for (int p = 0; p < g_nprods; p++) {
        int A = g_prods[p].lhs;
        int buf[MAX_SYMBOLS];
        int nb = 0;
        int nullseq = first_of_seq(g_prods[p].rhs, g_prods[p].len, buf, &nb);
        for (int i = 0; i < nb; i++) {
            int t = buf[i];
            if (g_table[A][t] != -1 && g_table[A][t] != p) {
                fprintf(stderr, "LL(1) conflict at M[%s, %s]: productions %d and %d\n",
                        g_symbols[A], g_symbols[t], g_table[A][t], p);
                g_conflicts++;
            }
            g_table[A][t] = p;
        }
        if (nullseq) {
            for (int i = 0; i < g_nfollow[A]; i++) {
                int t = g_follow[A][i];
                if (g_table[A][t] != -1 && g_table[A][t] != p) {
                    fprintf(stderr, "LL(1) conflict at M[%s, %s]: productions %d and %d\n",
                            g_symbols[A], g_symbols[t], g_table[A][t], p);
                    g_conflicts++;
                }
                g_table[A][t] = p;
            }
        }
    }
}

static void print_prod(int p) {
    printf("(%d) %s ->", p, g_symbols[g_prods[p].lhs]);
    if (g_prods[p].len == 0)
        printf(" epsilon");
    for (int i = 0; i < g_prods[p].len; i++)
        printf(" %s", g_symbols[g_prods[p].rhs[i]]);
    printf("\n");
}

static void print_set(const char *label, int sym, int *set, int n) {
    printf("  %s(%s) = { ", label, g_symbols[sym]);
    for (int i = 0; i < n; i++)
        printf("%s%s", g_symbols[set[i]], i + 1 < n ? ", " : "");
    if (n == 0)
        printf(" ");
    printf("}");
    if (strcmp(label, "FIRST") == 0)
        printf("  nullable=%d", g_nullable[sym] ? 1 : 0);
    printf("\n");
}

int main(int argc, char **argv) {
    FILE *in = stdin;
    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (!in) {
            perror(argv[1]);
            return 1;
        }
    } else {
        fprintf(stderr,
                "ll1_generic: reading grammar from stdin (or pass a grammar file as argv[1]).\n");
    }

    char line[1024];
    while (fgets(line, sizeof(line), in))
        parse_line(line);
    if (in != stdin)
        fclose(in);

    if (g_nprods == 0) {
        fprintf(stderr, "no productions parsed\n");
        return 1;
    }

    /* First pass collected NTs only via LHS; everything else is terminal. */
    compute_first();
    compute_follow();
    build_table();

    printf("================ Grammar (%d productions) ================\n", g_nprods);
    for (int p = 0; p < g_nprods; p++)
        print_prod(p);
    printf("Start symbol: %s\n", g_symbols[g_start]);

    printf("\n================ FIRST sets ================\n");
    for (int s = 0; s < g_nsyms; s++)
        if (g_is_nt[s])
            print_set("FIRST", s, g_first[s], g_nfirst[s]);

    printf("\n================ FOLLOW sets ================\n");
    for (int s = 0; s < g_nsyms; s++)
        if (g_is_nt[s])
            print_set("FOLLOW", s, g_follow[s], g_nfollow[s]);

    printf("\n================ LL(1) parsing table ================\n");
    /* Header row: list terminals (excluding NTs) */
    printf("%-8s|", "");
    for (int s = 0; s < g_nsyms; s++)
        if (!g_is_nt[s])
            printf(" %-6s", g_symbols[s]);
    printf("\n");
    for (int A = 0; A < g_nsyms; A++) {
        if (!g_is_nt[A])
            continue;
        printf("%-7s |", g_symbols[A]);
        for (int t = 0; t < g_nsyms; t++) {
            if (g_is_nt[t])
                continue;
            int p = g_table[A][t];
            if (p < 0)
                printf(" %-6s", "");
            else
                printf(" p%-5d", p);
        }
        printf("\n");
    }
    if (g_conflicts)
        printf("\n[!] %d LL(1) conflict(s) detected — grammar is NOT LL(1).\n", g_conflicts);
    else
        printf("\nGrammar is LL(1): no conflicts.\n");
    return 0;
}
