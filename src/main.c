#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "semantics.h"
#include "optimize.h"
#include "tac.h"
#include "token_dump.h"

extern FILE *yyin;
extern int yyparse(void);
extern AstNode *g_parse_result;

static void usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s [options] [file.mini]\n"
            "  -h            help\n"
            "  --pipeline    ONE input file → tokens + parse + semantics + TAC + passes\n"
            "                 (full compiler story in one run; requires a file)\n"
            "  --tokens      lexer only (token names)\n"
            "  --tac         three-address code only (default)\n"
            "  --opt         full optimisation passes after TAC\n"
            "  --check       semantic check only (no IR)\n",
            argv0);
}

int main(int argc, char **argv) {
    int opt_opt = 0;
    int check_only = 0;
    int tokens_only = 0;
    int pipeline = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--pipeline") == 0) {
            pipeline = 1;
            continue;
        }
        if (strcmp(argv[i], "--tokens") == 0) {
            tokens_only = 1;
            continue;
        }
        if (strcmp(argv[i], "--opt") == 0) {
            opt_opt = 1;
            continue;
        }
        if (strcmp(argv[i], "--check") == 0) {
            check_only = 1;
            continue;
        }
        if (strcmp(argv[i], "--tac") == 0)
            continue;
        if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
        path = argv[i];
    }

    if (pipeline) {
        opt_opt = 1;
        tokens_only = 0;
        check_only = 0;
        if (!path) {
            fprintf(stderr, "%s: --pipeline requires a source file (not stdin)\n", argv[0]);
            return 1;
        }
    }

    yyin = stdin;
    FILE *user_file = NULL;

    if (tokens_only) {
        if (!path) {
            user_file = NULL;
            yyin = stdin;
        } else {
            user_file = fopen(path, "r");
            if (!user_file) {
                perror(path);
                return 1;
            }
            yyin = user_file;
        }
        print_token_stream(yyin);
        if (user_file)
            fclose(user_file);
        return 0;
    }

    if (pipeline) {
        printf("========== Phase 1 — Lexical analysis (token stream) ==========\n");
        FILE *tf = fopen(path, "r");
        if (!tf) {
            perror(path);
            return 1;
        }
        print_token_stream(tf);
        fclose(tf);
        printf("\n========== Phase 2–6 — Syntax → semantics → IR (TAC) ==========\n");
        user_file = fopen(path, "r");
        if (!user_file) {
            perror(path);
            return 1;
        }
        yyin = user_file;
    } else if (path) {
        user_file = fopen(path, "r");
        if (!user_file) {
            perror(path);
            return 1;
        }
        yyin = user_file;
    }

    if (yyparse() != 0) {
        fprintf(stderr, "parse failed\n");
        if (user_file)
            fclose(user_file);
        return 1;
    }

    if (user_file)
        fclose(user_file);

    int se = semantic_check(g_parse_result);
    if (se != 0) {
        fprintf(stderr, "%d semantic error(s)\n", se);
        ast_free(g_parse_result);
        return 1;
    }

    if (check_only) {
        printf("semantic check passed.\n");
        ast_free(g_parse_result);
        return 0;
    }

    TacProgram *t = generate_tac(g_parse_result);
    printf("=== Three-address code (before optimisation) ===\n");
    tac_print(t);

    if (opt_opt) {
        if (pipeline)
            printf("\n========== Phase 7 — Optimisation passes ==========\n");
        else
            printf("\n=== Optimisation passes ===\n");
        optimize_tac(t);
        printf("\n=== Final TAC ===\n");
        tac_print(t);
        tac_free_program(t);
    } else {
        tac_free_program(t);
    }

    ast_free(g_parse_result);
    return 0;
}
