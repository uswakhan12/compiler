/*
 * Module 1 — Lab 05 Task 2: Postfix expression evaluator.
 *
 * Accepts non-negative integer operands, operators + - *, evaluates a
 * postfix expression using a stack, and prints every push/pop
 * transition. The final result is printed at end of input.
 *
 * Build:  gcc -std=c11 -O2 -o postfix_eval postfix_eval.c
 * Run:    echo "4 8 + 3 *" | ./postfix_eval
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_MAX 1024
static long stack[STACK_MAX];
static int sp = 0;

static void push(long v) {
    if (sp >= STACK_MAX) {
        fprintf(stderr, "stack overflow\n");
        exit(1);
    }
    stack[sp++] = v;
    printf("  push %ld     stack: [", v);
    for (int i = 0; i < sp; i++)
        printf("%s%ld", i ? " " : "", stack[i]);
    printf("]\n");
}

static long pop(void) {
    if (sp <= 0) {
        fprintf(stderr, "stack underflow\n");
        exit(1);
    }
    long v = stack[--sp];
    printf("  pop -> %ld    stack: [", v);
    for (int i = 0; i < sp; i++)
        printf("%s%ld", i ? " " : "", stack[i]);
    printf("]\n");
    return v;
}

static void apply(char op) {
    long b = pop();
    long a = pop();
    long r = 0;
    switch (op) {
    case '+':
        r = a + b;
        break;
    case '-':
        r = a - b;
        break;
    case '*':
        r = a * b;
        break;
    default:
        fprintf(stderr, "unsupported operator: %c\n", op);
        exit(1);
    }
    printf("  apply %c     %ld %c %ld = %ld\n", op, a, op, b, r);
    push(r);
}

int main(void) {
    printf("Postfix evaluator. Tokens separated by whitespace. Operators: + - *\n");
    int c;
    while ((c = getchar()) != EOF) {
        if (isspace(c))
            continue;
        if (isdigit(c)) {
            long n = c - '0';
            int nc;
            while ((nc = getchar()) != EOF && isdigit(nc))
                n = n * 10 + (nc - '0');
            if (nc != EOF)
                ungetc(nc, stdin);
            push(n);
        } else if (c == '+' || c == '-' || c == '*') {
            apply((char)c);
        } else {
            fprintf(stderr, "lexer error: unexpected character '%c'\n", c);
            return 1;
        }
    }
    if (sp != 1) {
        fprintf(stderr, "malformed expression (stack has %d items at EOF)\n", sp);
        return 1;
    }
    printf("Final result: %ld\n", stack[0]);
    return 0;
}
