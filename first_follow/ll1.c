/*
 * FIRST, FOLLOW, and LL(1) table for (Module 4):
 * E  -> T E'
 * E' -> + T E' | - T E' | ε
 * T  -> F T'
 * T' -> * F T' | / F T' | ε
 * F  -> ( E ) | id | num
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PLUS = 0,
    MINUS,
    STAR,
    SLASH,
    LP,
    RP,
    ID,
    NUM,
    END,
    NT_E = 9,
    NT_EP,
    NT_T,
    NT_TP,
    NT_F,
    SYM_MAX
};

static const char *symstr[] = {"+", "-", "*", "/", "(", ")", "id", "num", "$",
                               "E", "E'", "T", "T'", "F"};

static int is_term(int s) { return s < NT_E; }

#define MAXF 16
#define MAXP 16

static int first[SYM_MAX][MAXF];
static int nfirst[SYM_MAX];
static int follow[SYM_MAX][MAXF];
static int nfollow[SYM_MAX];
static int nullable_nt[SYM_MAX];

typedef struct {
    int id;
    int lhs;
    int rhs[6];
    int len;
} Prod;

static Prod prods[] = {
    {0, NT_E, {NT_T, NT_EP, -1}, 2},
    {1, NT_EP, {PLUS, NT_T, NT_EP, -1}, 3},
    {2, NT_EP, {MINUS, NT_T, NT_EP, -1}, 3},
    {3, NT_EP, {-1}, 0},
    {4, NT_T, {NT_F, NT_TP, -1}, 2},
    {5, NT_TP, {STAR, NT_F, NT_TP, -1}, 3},
    {6, NT_TP, {SLASH, NT_F, NT_TP, -1}, 3},
    {7, NT_TP, {-1}, 0},
    {8, NT_F, {LP, NT_E, RP, -1}, 3},
    {9, NT_F, {ID, -1}, 1},
    {10, NT_F, {NUM, -1}, 1},
};

#define NPROD (sizeof(prods) / sizeof(prods[0]))

static void add_unique(int *arr, int *n, int t) {
    for (int i = 0; i < *n; i++)
        if (arr[i] == t)
            return;
    arr[(*n)++] = t;
}

static void first_of_seq(int *rhs, int len, int *out, int *nout, int *maybe_empty) {
    *nout = 0;
    *maybe_empty = 1;
    for (int i = 0; i < len; i++) {
        int s = rhs[i];
        if (s < 0)
            break;
        if (is_term(s)) {
            add_unique(out, nout, s);
            *maybe_empty = 0;
            return;
        }
        for (int k = 0; k < nfirst[s]; k++)
            add_unique(out, nout, first[s][k]);
        if (!nullable_nt[s]) {
            *maybe_empty = 0;
            return;
        }
    }
}

static void compute_first(void) {
    memset(nfirst, 0, sizeof(nfirst));
    memset(nullable_nt, 0, sizeof(nullable_nt));
    for (int t = PLUS; t <= NUM; t++) {
        nfirst[t] = 1;
        first[t][0] = t;
    }
    int chg = 1;
    while (chg) {
        chg = 0;
        for (size_t p = 0; p < NPROD; p++) {
            int lhs = prods[p].lhs;
            int old = nfirst[lhs];
            if (prods[p].len == 0) {
                nullable_nt[lhs] = 1;
                continue;
            }
            int buf[MAXF], nb = 0, me;
            first_of_seq(prods[p].rhs, prods[p].len, buf, &nb, &me);
            if (me)
                nullable_nt[lhs] = 1;
            for (int i = 0; i < nb; i++)
                add_unique(first[lhs], &nfirst[lhs], buf[i]);
            if (nfirst[lhs] != old)
                chg = 1;
        }
    }
}

static void compute_follow(void) {
    memset(nfollow, 0, sizeof(nfollow));
    add_unique(follow[NT_E], &nfollow[NT_E], END);
    int chg = 1;
    while (chg) {
        chg = 0;
        for (size_t p = 0; p < NPROD; p++) {
            int lhs = prods[p].lhs;
            int *R = prods[p].rhs;
            int L = prods[p].len;
            for (int i = 0; i < L; i++) {
                int B = R[i];
                if (is_term(B))
                    continue;
                int tail[MAXF], nt = 0, me;
                first_of_seq(R + i + 1, L - i - 1, tail, &nt, &me);
                int oldb = nfollow[B];
                for (int k = 0; k < nt; k++)
                    add_unique(follow[B], &nfollow[B], tail[k]);
                if (me || i == L - 1) {
                    for (int k = 0; k < nfollow[lhs]; k++)
                        add_unique(follow[B], &nfollow[B], follow[lhs][k]);
                }
                if (nfollow[B] != oldb)
                    chg = 1;
            }
        }
    }
}

static int table[NT_F + 1][END + 1];

static void build_table(void) {
    memset(table, 0xff, sizeof(table));
    for (size_t p = 0; p < NPROD; p++) {
        int A = prods[p].lhs;
        int buf[MAXF], nb = 0, me;
        first_of_seq(prods[p].rhs, prods[p].len, buf, &nb, &me);
        for (int i = 0; i < nb; i++) {
            int t = buf[i];
            if (t <= END && A >= NT_E)
                table[A][t] = (int)p;
        }
        if (me) {
            for (int i = 0; i < nfollow[A]; i++) {
                int t = follow[A][i];
                if (t <= END && A >= NT_E)
                    table[A][t] = (int)p;
            }
        }
    }
}

int main(void) {
    compute_first();
    compute_follow();
    build_table();

    printf("FIRST sets:\n");
    for (int nt = NT_E; nt <= NT_F; nt++) {
        printf("  FIRST(%s) = { ", symstr[nt]);
        for (int i = 0; i < nfirst[nt]; i++) {
            printf("%s", symstr[first[nt][i]]);
            if (i + 1 < nfirst[nt])
                printf(", ");
        }
        printf(" }  nullable=%d\n", nullable_nt[nt]);
    }

    printf("\nFOLLOW sets:\n");
    for (int nt = NT_E; nt <= NT_F; nt++) {
        printf("  FOLLOW(%s) = { ", symstr[nt]);
        for (int i = 0; i < nfollow[nt]; i++) {
            printf("%s", symstr[follow[nt][i]]);
            if (i + 1 < nfollow[nt])
                printf(", ");
        }
        printf(" }\n");
    }

    printf("\nLL(1) parsing table M[nonterminal, terminal] -> production #:\n");
    printf("%-6s", "");
    for (int t = PLUS; t <= END; t++)
        printf("%6s", symstr[t]);
    printf("\n");
    for (int nt = NT_E; nt <= NT_F; nt++) {
        printf("%-5s|", symstr[nt]);
        for (int t = PLUS; t <= END; t++) {
            int v = table[nt][t];
            if (v < 0)
                printf("%6s", "");
            else
                printf("%6d", v);
        }
        printf("\n");
    }

    return 0;
}
