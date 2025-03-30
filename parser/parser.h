#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer/lexer.h"

typedef struct Node {
    char *value;
    char *description;
    struct Node *parent;
    struct Node **children;
    int numChildren;
} Node;

extern Node *root;

void printTreeWithBranches(Node *node, int depth, int is_last[]);
void addNodes(Node *base, Token *token);
Node *createNode(char *value, char *description);
void printTreePretty(Node *root);
Node *allocNode(Node *base, Node *newNode);
void callAnteriorAndNext(Node *newNode, Token *token);

#endif 