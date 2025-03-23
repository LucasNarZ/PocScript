#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define NUM_TYPES 7

typedef struct Token {
    char *type;
    char *value;
    struct Token *next;
    struct Token *anterior;
} Token;

typedef struct {
    char *type;
    char *pattern;
} TokenType;

// Declaração das variáveis globais
extern Token *head;
extern TokenType types[NUM_TYPES];

// Declaração das funções
void getTokens(Token **head, char **input);
void print(Token *head);
void printInv(Token *head);

#endif // LEXER_H
