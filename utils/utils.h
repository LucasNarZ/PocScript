#include <stdio.h>
#include <stdbool.h>
#include "../types.h"

#ifndef UTILS_H
#define UTILS_H

void printStack(Stack *stack);
void printScopeStack(ScopeStack *stack);
bool arrayContains(char *array[], int tamanho, const char *valor);
void printTreePretty(Node *root);
void printTreeWithBranches(Node *node, int depth, int is_last[]);
void popVaribles(Stack *stack, int value);

#endif