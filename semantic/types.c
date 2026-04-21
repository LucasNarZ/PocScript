#include "types.h"

#include <stdlib.h>

SemanticType *semanticTypeNewPrimitive(SemanticTypeKind kind) {
    SemanticType *type = malloc(sizeof(SemanticType));

    if (type == NULL) {
        return NULL;
    }

    type->kind = kind;
    type->element_type = NULL;
    return type;
}
SemanticType *semanticTypeClone(const SemanticType *type) {
    SemanticType *copy;

    if (type == NULL) {
        return NULL;
    }

    copy = semanticTypeNewPrimitive(type->kind);
    if (copy == NULL) {
        return NULL;
    }

    if (type->element_type != NULL) {
        copy->element_type = semanticTypeClone(type->element_type);
    }

    return copy;
}

void semanticTypeFree(SemanticType *type) {
    if (type == NULL) {
        return;
    }

    semanticTypeFree(type->element_type);
    free(type);
}

bool semanticTypeEquals(const SemanticType *left, const SemanticType *right) {
    if (left == NULL || right == NULL) {
        return false;
    }

    if (left->kind != right->kind) {
        return false;
    }

    if (left->kind != SEM_TYPE_ARRAY) {
        return true;
    }

    return semanticTypeEquals(left->element_type, right->element_type);
}

const char *semanticTypeName(const SemanticType *type) {
    if (type == NULL) {
        return "unknown";
    }

    switch (type->kind) {
        case SEM_TYPE_INT:
            return "int";
        case SEM_TYPE_FLOAT:
            return "float";
        case SEM_TYPE_CHAR:
            return "char";
        case SEM_TYPE_BOOL:
            return "bool";
        case SEM_TYPE_VOID:
            return "void";
        case SEM_TYPE_ARRAY:
            return "array";
        case SEM_TYPE_ERROR:
            return "error";
    }

    return "unknown";
}

SemanticType *semanticTypeFromAst(AstNode *typeNode) {
    SemanticType *type;

    if (typeNode == NULL) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    if (typeNode->type == AST_TYPE_ARRAY) {
        type = semanticTypeNewPrimitive(SEM_TYPE_ARRAY);
        if (type == NULL) {
            return NULL;
        }

        type->element_type = semanticTypeFromAst(typeNode->data.type_array.element_type);
        return type;
    }

    if (typeNode->type != AST_TYPE_NAME) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    switch (typeNode->data.type_name.kind) {
        case AST_TYPE_INT_KIND:
            return semanticTypeNewPrimitive(SEM_TYPE_INT);
        case AST_TYPE_FLOAT_KIND:
            return semanticTypeNewPrimitive(SEM_TYPE_FLOAT);
        case AST_TYPE_CHAR_KIND:
            return semanticTypeNewPrimitive(SEM_TYPE_CHAR);
        case AST_TYPE_BOOL_KIND:
            return semanticTypeNewPrimitive(SEM_TYPE_BOOL);
        case AST_TYPE_VOID_KIND:
            return semanticTypeNewPrimitive(SEM_TYPE_VOID);
        case AST_TYPE_ARRAY_KIND:
            type = semanticTypeNewPrimitive(SEM_TYPE_ARRAY);
            if (type == NULL) {
                return NULL;
            }
            type->element_type = semanticTypeNewPrimitive(SEM_TYPE_ERROR);
            return type;
        case AST_TYPE_CUSTOM_KIND:
        default:
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }
}
