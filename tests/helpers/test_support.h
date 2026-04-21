#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "../../lexer/lexer.h"
#include "../../parser/ast.h"
#include "../../parser/parser.h"
#include "../../semantic/semantic.h"

AstNode *parseRootFromString(const char *input);
char *nodeTreeToString(AstNode *root);
SemanticResult analyzeRootFromString(const char *input);

#endif
