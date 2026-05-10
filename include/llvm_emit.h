#ifndef LLVM_EMIT_H
#define LLVM_EMIT_H

#include "ast.h"
#include "tac.h"
#include <stdio.h>

/* Module 8 — emit textual LLVM IR for the given TAC program.
   The AST is consulted to recover declared variable types
   (int → i32, float → double). The output is a self-contained
   `define i32 @main()` function plus the required printf prototype. */
void emit_llvm_ir(FILE *out, AstNode *program, TacProgram *tac);

#endif
