#ifndef PARSER_H
#define PARSER_H

#define MAX_SIZE_STACK 100
#define MAX_SIZE_SCOPE 50

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../lexer/lexer.h"

typedef struct Node {
    char *value;
    char *description;
    struct Node *parent;
    struct Node **children;
    int numChildren;
} Node;

typedef struct Stack {
    char **variables[MAX_SIZE_STACK];
    int size;
} Stack;

typedef struct ScopeStack {
    int *scope;
    int size;
} ScopeStack;

extern Stack stack;
extern ScopeStack scopesStack;

bool arrayContains(char *array[], int tamanho, const char *valor);
void printTreeWithBranches(Node *node, int depth, int is_last[]);
Node *parseFactor(Token **token);
Node *parseTerm(Token **token);
Node *parseExpression(Token **token);
Node *parseStatement(Node *root, Token **token);
Node *parseComparison(Token **token);
Node *parseLogical(Token **token);
Node *parseAssign(Token **token);
Node *parseBlock(Node *root, Token **token);
Node *parseArguments(Node *func, Token **token);
Node *createNode(char *value, char *description);
void printTreePretty(Node *root);
Node *allocNode(Node *base, Node *newNode);

#endif 