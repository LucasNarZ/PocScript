#include "scope.h"

#include <stdlib.h>
#include <string.h>

static size_t hashName(const char *name, size_t bucketCount) {
    size_t hash = 5381;

    while (*name != '\0') {
        hash = ((hash << 5) + hash) + (unsigned char) *name;
        name++;
    }

    return hash % bucketCount;
}

static char *copyName(const char *name) {
    size_t length = strlen(name);
    char *copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, name, length + 1);
    return copy;
}

Scope *scopeCreate(Scope *parent) {
    Scope *scope = calloc(1, sizeof(Scope));

    if (scope == NULL) {
        return NULL;
    }

    scope->bucket_count = 64;
    scope->buckets = calloc(scope->bucket_count, sizeof(Symbol *));
    if (scope->buckets == NULL) {
        free(scope);
        return NULL;
    }

    scope->parent = parent;
    return scope;
}

void symbolFree(Symbol *symbol) {
    size_t i;

    if (symbol == NULL) {
        return;
    }

    free(symbol->name);
    semanticTypeFree(symbol->type);
    for (i = 0; i < symbol->param_count; i++) {
        semanticTypeFree(symbol->params[i]);
    }
    free(symbol->params);
    free(symbol);
}

void scopeFree(Scope *scope) {
    size_t i;

    if (scope == NULL) {
        return;
    }

    for (i = 0; i < scope->bucket_count; i++) {
        Symbol *current = scope->buckets[i];
        while (current != NULL) {
            Symbol *next = current->next;
            symbolFree(current);
            current = next;
        }
    }

    free(scope->buckets);
    free(scope);
}

Symbol *scopeLookupCurrent(Scope *scope, const char *name) {
    Symbol *current;
    size_t index;

    if (scope == NULL) {
        return NULL;
    }

    index = hashName(name, scope->bucket_count);
    current = scope->buckets[index];
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

Symbol *scopeLookup(Scope *scope, const char *name) {
    while (scope != NULL) {
        Symbol *symbol = scopeLookupCurrent(scope, name);
        if (symbol != NULL) {
            return symbol;
        }
        scope = scope->parent;
    }

    return NULL;
}

bool scopeDeclare(Scope *scope, Symbol *symbol) {
    size_t index;

    if (scope == NULL || symbol == NULL) {
        return false;
    }

    if (scopeLookupCurrent(scope, symbol->name) != NULL) {
        return false;
    }

    index = hashName(symbol->name, scope->bucket_count);
    symbol->next = scope->buckets[index];
    scope->buckets[index] = symbol;
    return true;
}

Symbol *symbolCreateVariable(const char *name, SemanticType *type) {
    Symbol *symbol = calloc(1, sizeof(Symbol));

    if (symbol == NULL) {
        semanticTypeFree(type);
        return NULL;
    }

    symbol->name = copyName(name);
    if (symbol->name == NULL) {
        semanticTypeFree(type);
        free(symbol);
        return NULL;
    }

    symbol->kind = SYMBOL_VARIABLE;
    symbol->type = type;
    return symbol;
}

Symbol *symbolCreateFunction(const char *name, SemanticType *returnType, SemanticType **params, size_t param_count) {
    Symbol *symbol = calloc(1, sizeof(Symbol));
    size_t i;

    if (symbol == NULL) {
        semanticTypeFree(returnType);
        return NULL;
    }

    symbol->name = copyName(name);
    if (symbol->name == NULL) {
        semanticTypeFree(returnType);
        free(symbol);
        return NULL;
    }

    symbol->type = semanticTypeClone(returnType);
    semanticTypeFree(returnType);
    if (symbol->type == NULL) {
        free(symbol->name);
        free(symbol);
        return NULL;
    }

    if (param_count > 0) {
        symbol->params = calloc(param_count, sizeof(SemanticType *));
        if (symbol->params == NULL) {
            semanticTypeFree(symbol->type);
            free(symbol->name);
            free(symbol);
            return NULL;
        }
    }

    for (i = 0; i < param_count; i++) {
        symbol->params[i] = semanticTypeClone(params[i]);
    }

    symbol->kind = SYMBOL_FUNCTION;
    symbol->param_count = param_count;
    return symbol;
}
