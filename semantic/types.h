#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <stdbool.h>

#include "../parser/ast.h"

typedef enum {
    SEM_TYPE_INT,
    SEM_TYPE_FLOAT,
    SEM_TYPE_CHAR,
    SEM_TYPE_BOOL,
    SEM_TYPE_STRING,
    SEM_TYPE_VOID,
    SEM_TYPE_ARRAY,
    SEM_TYPE_ERROR
} SemanticTypeKind;

typedef struct SemanticType {
    SemanticTypeKind kind;
    struct SemanticType *element_type;
} SemanticType;

SemanticType *semanticTypeNewPrimitive(SemanticTypeKind kind);
SemanticType *semanticTypeFromAst(AstNode *typeNode);
SemanticType *semanticTypeClone(const SemanticType *type);
void semanticTypeFree(SemanticType *type);
bool semanticTypeEquals(const SemanticType *left, const SemanticType *right);
const char *semanticTypeName(const SemanticType *type);

#endif
