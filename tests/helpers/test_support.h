#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "semantic.h"
#include "ir.h"

AstNode *parseRootFromString(const char *input);
char *nodeTreeToString(AstNode *root);
SemanticResult analyzeRootFromString(const char *input);
IRModule *buildIrModuleFromString(const char *input);
char *emitLlvmIrFromString(const char *input);

#endif
