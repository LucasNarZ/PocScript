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
extern TokenType types[NUM_TYPES];

void getTokens(Token **head, char **input);
void print(Token *head);
void printInv(Token *head);

#endif 
