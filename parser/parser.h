#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer/token.h"
#include "ast.h"

typedef struct {
    Token *tokens;
    Token *current;
} Parser;

void parserInit(Parser *parser, Token *tokens);
AstNode *parserParseProgram(Parser *parser);

#endif 
