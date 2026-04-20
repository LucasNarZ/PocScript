#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include "token.h"

extern Token *head;
extern TokenDefinition types[NUM_TYPES];

void getTokens(Token **head, char **input, int *line, int *column);
const char *tokenTypeName(TokenType type);
Token *tokenizeString(const char *input);
Token *tokenizeFile(const char *path);
char *readFileToString(const char *path);
char *tokensToString(Token *head);
char *tokenizeFileToString(const char *path);
void freeTokens(Token *head);
void print(Token *head);
void printInv(Token *head);

#endif 
