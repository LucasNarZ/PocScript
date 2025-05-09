#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../types.h"
#include "../constants.h"
#include "../utils/utils.h"
#include "../parser/parser.h"
#include "./hashTable.h"

extern Stack stack;
extern ScopeStack scopesStack;
extern LineIndices *functionLineIndices;
extern LineIndices *lineIndices;

void generateAssembly(Node *root, FILE *outputFile, LineIndices *lineIndices);
void writeFile(const char *filename, char **lines, LineIndices *lineIndices, char **functionLines, LineIndices *functionLineIndices);
void writeAtLine(const char *text, char **lines, LineIndices *lineIndices, int lineIndex);
void writeAssignGlobalVaribleInt(const char *name, const char *value, char **lines, LineIndices *lineIndices);
void walkTree(Node *node, char **lines, LineIndices *lineIndices);

#endif