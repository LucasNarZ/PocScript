#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "ast.h"

typedef struct {
    int line;
    int column;
    char *message;
} ParserError;

typedef struct {
    ParserError *items;
    size_t count;
    size_t capacity;
} ParserErrorList;

typedef struct {
    Token *tokens;
    Token *current;
    ParserErrorList errors;
    bool is_panic;
} Parser;

void parserInit(Parser *parser, Token *tokens);
void parserFree(Parser *parser);
AstNode *parserParseProgram(Parser *parser);

#endif 
