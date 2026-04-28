#include "errors.h"

#include <stdlib.h>
#include <string.h>

static char *copyMessage(const char *message) {
    size_t length = strlen(message);
    char *copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, message, length + 1);
    return copy;
}

const char *semanticErrorKindName(SemanticErrorKind kind) {
    switch (kind) {
        case SEMANTIC_ERROR_TYPE:
            return "TypeError";
        case SEMANTIC_ERROR_NAME:
            return "NameError";
        case SEMANTIC_ERROR_DECLARATION:
            return "DeclarationError";
        case SEMANTIC_ERROR_DEVELOPER:
            return "DeveloperError";
    }

    return "SemanticError";
}

void semanticErrorListInit(SemanticErrorList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void semanticErrorListAppend(SemanticErrorList *list, SemanticErrorKind kind, int line, int column, const char *message) {
    SemanticError *nextItems;
    char *copy;
    size_t nextCapacity;

    if (list->count == list->capacity) {
        nextCapacity = list->capacity == 0 ? 4 : list->capacity * 2;
        nextItems = realloc(list->items, nextCapacity * sizeof(SemanticError));
        if (nextItems == NULL) {
            return;
        }

        list->items = nextItems;
        list->capacity = nextCapacity;
    }

    copy = copyMessage(message);
    if (copy == NULL) {
        return;
    }

    list->items[list->count].kind = kind;
    list->items[list->count].line = line;
    list->items[list->count].column = column;
    list->items[list->count].message = copy;
    list->count++;
}

void semanticErrorListFree(SemanticErrorList *list) {
    size_t i;

    for (i = 0; i < list->count; i++) {
        free(list->items[i].message);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
