#include "constants.h"

#ifndef TYPE_H
#define TYPE_H

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
    int scope[MAX_SIZE_SCOPE];
    int size;
} ScopeStack;

#endif
