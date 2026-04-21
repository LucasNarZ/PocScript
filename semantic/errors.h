#ifndef SEMANTIC_ERRORS_H
#define SEMANTIC_ERRORS_H

#include <stddef.h>

typedef enum {
    SEMANTIC_ERROR_TYPE,
    SEMANTIC_ERROR_NAME,
    SEMANTIC_ERROR_DECLARATION
} SemanticErrorKind;

typedef struct {
    SemanticErrorKind kind;
    int line;
    int column;
    char *message;
} SemanticError;

typedef struct {
    SemanticError *items;
    size_t count;
    size_t capacity;
} SemanticErrorList;

void semanticErrorListInit(SemanticErrorList *list);
void semanticErrorListAppend(SemanticErrorList *list, SemanticErrorKind kind, int line, int column, const char *message);
void semanticErrorListFree(SemanticErrorList *list);
const char *semanticErrorKindName(SemanticErrorKind kind);

#endif
