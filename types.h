#include "constants.h"

#ifndef TYPE_H
#define TYPE_H

typedef enum {
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_COMMENT,
    TOKEN_TYPE_ARRAY,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_CHAR,
    TOKEN_TYPE_BOOL,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_FOR,
    TOKEN_KW_WHILE,
    TOKEN_KW_FUNC,
    TOKEN_KW_RET,
    TOKEN_STRING,
    TOKEN_EQ_EQ,
    TOKEN_GT_EQ,
    TOKEN_LT_EQ,
    TOKEN_NOT_EQ,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_BANG,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_ASSIGN,
    TOKEN_DOUBLE_COLON,
    TOKEN_IDENTIFIER,
    TOKEN_OR_OR,
    TOKEN_AND_AND,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_DELIMITER,
    TOKEN_WHITESPACE
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    struct Token *next;
    struct Token *anterior;
} Token;

typedef struct {
    TokenType type;
    char *pattern;
} TokenDefinition;

typedef struct Node {
    char *value;
    char *description;
    struct Node *parent;
    struct Node **children;
    int numChildren;
} Node;


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
