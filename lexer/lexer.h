#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include "../utils/utils.h"
#include "../types.h"

extern Token *head;
extern TokenDefinition types[NUM_TYPES];

void getTokens(Token **head, char **input);
const char *tokenTypeName(TokenType type);
Token *tokenizeString(const char *input);
char *readFileToString(const char *path);
char *tokensToString(Token *head);
char *tokenizeFileToString(const char *path);
void freeTokens(Token *head);
void print(Token *head);
void printInv(Token *head);

#endif 
