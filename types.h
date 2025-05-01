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

// typedef struct typeNode {
//     union {
//         char *type;
//         struct typeNode *typeNode;
//     };
    
// } TypeNode;

typedef struct Node {
    char *value;
    char *description;
    struct Node *parent;
    struct Node **children;
    int numChildren;
} Node;

// typedef struct literalNode {
//     char *value;
//     struct typeNode *type;
// } LiteralNode;

// typedef struct assignmentNode {
//     char *value;
//     struct variableNode *var;
//     struct literalNode *literal;
// } AssignmentNode;

// typedef struct operatorNode {
//     char *value;
//     struct literalNode *left;
//     struct literalNode *right;
// } OperatorNode;

// typedef struct arrayNode {
//     char *value;
//     union {
//         struct literalNode *literal;
//         struct variableNode *var;
//         struct arrayNode *array;
//     } data;
//     struct typeNode *type;
// } ArrayNode;

// typedef struct functionNode {
//     char *value;
//     struct variableNode *var;
//     struct literalNode *literal;
// } FunctionNode;

// typedef struct logicalNode {
//     char *value;
//     union {
//         struct logicalNode *logical;
//         struct comparisonNode *comparison;
//         struct literalNode *literal;
//     };
// } LogicalNode;

// typedef struct comparisonNode {
//     char *value;
//     union {
//         struct comparisonNode *comparison;
//         struct operatorNode *operator;
//         struct literalNode *literal;
//     };
// } ComparisonNode;


// typedef struct primitiveVariableNode {
//     char *value;
//     struct typeNode *type;
// } PrimitiveVariableNode;

typedef struct variable {
    char *name;
    struct Node *type;
} Variable;

typedef struct Stack {
    struct variable *variables[MAX_SIZE_STACK];
    int size;
} Stack;

typedef struct ScopeStack {
    int scope[MAX_SIZE_SCOPE];
    int size;
} ScopeStack;

typedef struct lineIndices {
    int currentLine;
    int globalVariblesLine;
    int pastInstructionLine;
} LineIndices;

typedef struct pair {
    char *key;
    char *value;
    struct pair *next;
} Pair;

#endif
