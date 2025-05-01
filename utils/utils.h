#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../types.h"

#ifndef UTILS_H
#define UTILS_H

char *getVarType(char *var, Stack *stack);
void defineVariables(char *name, Node *type, Stack *stack, ScopeStack *scopesStack);
void getVarsNames(char **names, Stack *stack);
Variable *createVariable(char *name, Node *type);
void printType(Node *typeNode);
void printStack(Stack *stack);
void printScopeStack(ScopeStack *stack);
bool arrayContains(char *array[], int tamanho, const char *valor);
void printTreePretty(Node *root);
void printTreeWithBranches(Node *node, int depth, int is_last[]);
void popVaribles(Stack *stack, int value);
void cleanStack(Stack *stack, ScopeStack *ScopeStack);
void printHashTable(Pair *hashTable[HASH_TABLE_SIZE]);

#endif