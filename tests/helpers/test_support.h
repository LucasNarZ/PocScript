#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "../../lexer/lexer.h"
#include "../../parser/ast.h"
#include "../../parser/parser.h"

AstNode *parseRootFromString(const char *input);
char *nodeTreeToString(AstNode *root);

#endif
