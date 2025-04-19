#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/utils.h"
#include "../types.h"
#include "ast.h"

extern Stack stack;
extern ScopeStack scopesStack;

// Variable *createVariable(char *name, char *type);
Node *parseFactor(Token **token);
Node *parseTerm(Token **token);
Node *parseExpression(Token **token);
Node *parseStatement(Node *root, Token **token);
Node *parseComparison(Token **token);
Node *parseLogical(Token **token);
Node *parseAssign(Token **token);
Node *parseBlock(Node *root, Token **token);
void parseArguments(Node *func, Token **token);
void parseType(Node *var, Token **token);
void parseArray(Node *node, Token **token);
void parseCallArguments(Node *func, Token **token);

#endif 