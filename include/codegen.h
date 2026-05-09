#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "tac.h"

TacProgram *generate_tac(AstNode *prog);

#endif
