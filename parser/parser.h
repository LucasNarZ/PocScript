#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer/token.h"
#include "ast.h"

AstNode *parseProgram(Token **token);
AstNode *parseBlock(Token **token);
AstNode *parseStatement(Token **token);
AstNode *parseReturn(Token **token);
AstNode *parseAssign(Token **token);
AstNode *parseLogical(Token **token);
AstNode *parseComparison(Token **token);
AstNode *parseExpression(Token **token);
AstNode *parseTerm(Token **token);
AstNode *parseNegation(Token **token);
AstNode *parseFactor(Token **token);
AstNode *parseType(Token **token);
AstNode *parseArrayLiteral(Token **token);

#endif 
