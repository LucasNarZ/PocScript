#ifndef SEMANTIC_SCOPE_H
#define SEMANTIC_SCOPE_H

#include <stdbool.h>
#include <stddef.h>

#include "types.h"

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION
} SymbolKind;

typedef struct Symbol {
    char *name;
    SymbolKind kind;
    SemanticType *type;
    SemanticType **params;
    size_t param_count;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    struct Scope *parent;
    Symbol **buckets;
    size_t bucket_count;
} Scope;

Scope *scopeCreate(Scope *parent);
void scopeFree(Scope *scope);
Symbol *scopeLookupCurrent(Scope *scope, const char *name);
Symbol *scopeLookup(Scope *scope, const char *name);
bool scopeDeclare(Scope *scope, Symbol *symbol);
Symbol *symbolCreateVariable(const char *name, SemanticType *type);
Symbol *symbolCreateFunction(const char *name, SemanticType *returnType, SemanticType **params, size_t param_count);
void symbolFree(Symbol *symbol);

#endif
