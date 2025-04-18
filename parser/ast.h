#include "../types.h"

#ifndef AST_H
#define AST_H

Node *createNode(char *value, char *description);
Node *allocNode(Node *base, Node *newNode);

#endif