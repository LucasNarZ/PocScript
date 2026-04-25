#include "ir.h"

#include <stdlib.h>
#include <string.h>

static const size_t IR_DEFAULT_BUCKET_COUNT = 64;

static char *irCopyString(const char *value) {
    char *copy;
    size_t length;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value);
    copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, value, length + 1);
    return copy;
}

static size_t irHashName(const char *name, size_t bucket_count) {
    size_t hash = 5381;

    while (*name != '\0') {
        hash = ((hash << 5) + hash) + (unsigned char) *name;
        name++;
    }

    return hash % bucket_count;
}

IRSymbol *irSymbolCreateGlobal(const char *name, IRType *type, unsigned int global_id) {
    IRSymbol *symbol = calloc(1, sizeof(IRSymbol));

    if (symbol == NULL) {
        irTypeFree(type);
        return NULL;
    }

    symbol->name = irCopyString(name);
    if (name != NULL && symbol->name == NULL) {
        irTypeFree(type);
        free(symbol);
        return NULL;
    }

    symbol->kind = IR_SYMBOL_GLOBAL;
    symbol->type = type;
    symbol->data.global_id = global_id;
    return symbol;
}

IRSymbol *irSymbolCreateLocal(const char *name, IRType *type, unsigned int local_id) {
    IRSymbol *symbol = calloc(1, sizeof(IRSymbol));

    if (symbol == NULL) {
        irTypeFree(type);
        return NULL;
    }

    symbol->name = irCopyString(name);
    if (name != NULL && symbol->name == NULL) {
        irTypeFree(type);
        free(symbol);
        return NULL;
    }

    symbol->kind = IR_SYMBOL_LOCAL;
    symbol->type = type;
    symbol->data.local_id = local_id;
    return symbol;
}

IRSymbol *irSymbolCreateFunction(const char *name, IRType *return_type, IRType **param_types, size_t param_count) {
    IRSymbol *symbol = calloc(1, sizeof(IRSymbol));
    size_t i;

    if (symbol == NULL) {
        irTypeFree(return_type);
        return NULL;
    }

    symbol->name = irCopyString(name);
    if (name != NULL && symbol->name == NULL) {
        irTypeFree(return_type);
        free(symbol);
        return NULL;
    }

    symbol->kind = IR_SYMBOL_FUNCTION;
    symbol->type = return_type;
    symbol->data.function.param_count = param_count;
    symbol->data.function.return_type = irTypeClone(return_type);
    if (symbol->data.function.return_type == NULL) {
        irSymbolFree(symbol);
        return NULL;
    }

    if (param_count > 0) {
        symbol->data.function.param_types = calloc(param_count, sizeof(IRType *));
        if (symbol->data.function.param_types == NULL) {
            irSymbolFree(symbol);
            return NULL;
        }
    }

    for (i = 0; i < param_count; i++) {
        symbol->data.function.param_types[i] = irTypeClone(param_types[i]);
        if (param_types[i] != NULL && symbol->data.function.param_types[i] == NULL) {
            irSymbolFree(symbol);
            return NULL;
        }
    }

    return symbol;
}

void irSymbolFree(IRSymbol *symbol) {
    size_t i;

    if (symbol == NULL) {
        return;
    }

    free(symbol->name);
    irTypeFree(symbol->type);
    if (symbol->kind == IR_SYMBOL_FUNCTION) {
        irTypeFree(symbol->data.function.return_type);
        for (i = 0; i < symbol->data.function.param_count; i++) {
            irTypeFree(symbol->data.function.param_types[i]);
        }
        free(symbol->data.function.param_types);
    }
    free(symbol);
}

IRSymbolTable *irSymbolTableCreate(size_t bucket_count) {
    IRSymbolTable *table = calloc(1, sizeof(IRSymbolTable));

    if (table == NULL) {
        return NULL;
    }

    table->bucket_count = bucket_count == 0 ? IR_DEFAULT_BUCKET_COUNT : bucket_count;
    table->buckets = calloc(table->bucket_count, sizeof(IRSymbol *));
    if (table->buckets == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

void irSymbolTableFree(IRSymbolTable *table) {
    size_t i;

    if (table == NULL) {
        return;
    }

    for (i = 0; i < table->bucket_count; i++) {
        IRSymbol *current = table->buckets[i];
        while (current != NULL) {
            IRSymbol *next = current->next;
            irSymbolFree(current);
            current = next;
        }
    }

    free(table->buckets);
    free(table);
}

IRSymbol *irSymbolTableLookupCurrent(IRSymbolTable *table, const char *name) {
    IRSymbol *current;
    size_t index;

    if (table == NULL || name == NULL) {
        return NULL;
    }

    index = irHashName(name, table->bucket_count);
    current = table->buckets[index];
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

bool irSymbolTableDeclare(IRSymbolTable *table, IRSymbol *symbol) {
    size_t index;

    if (table == NULL || symbol == NULL) {
        return false;
    }

    if (irSymbolTableLookupCurrent(table, symbol->name) != NULL) {
        return false;
    }

    index = irHashName(symbol->name, table->bucket_count);
    symbol->next = table->buckets[index];
    table->buckets[index] = symbol;
    return true;
}

IRScope *irScopeCreate(IRScope *parent) {
    IRScope *scope = calloc(1, sizeof(IRScope));

    if (scope == NULL) {
        return NULL;
    }

    scope->parent = parent;
    scope->symbols = irSymbolTableCreate(IR_DEFAULT_BUCKET_COUNT);
    if (scope->symbols == NULL) {
        free(scope);
        return NULL;
    }

    return scope;
}

void irScopeFree(IRScope *scope) {
    if (scope == NULL) {
        return;
    }

    irSymbolTableFree(scope->symbols);
    free(scope);
}

IRSymbol *irScopeLookupCurrent(IRScope *scope, const char *name) {
    if (scope == NULL) {
        return NULL;
    }

    return irSymbolTableLookupCurrent(scope->symbols, name);
}

IRSymbol *irScopeLookup(IRScope *scope, const char *name) {
    while (scope != NULL) {
        IRSymbol *symbol = irScopeLookupCurrent(scope, name);
        if (symbol != NULL) {
            return symbol;
        }
        scope = scope->parent;
    }

    return NULL;
}

bool irScopeDeclare(IRScope *scope, IRSymbol *symbol) {
    if (scope == NULL) {
        return false;
    }

    return irSymbolTableDeclare(scope->symbols, symbol);
}
