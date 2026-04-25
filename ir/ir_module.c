#include "ir.h"

#include <stdlib.h>
#include <string.h>

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

static bool irGrowPointerArray(void ***items, size_t *capacity, size_t item_size) {
    void **grown_items;
    size_t new_capacity;

    new_capacity = *capacity == 0 ? 4 : (*capacity * 2);
    grown_items = realloc(*items, new_capacity * item_size);
    if (grown_items == NULL) {
        return false;
    }

    *items = grown_items;
    *capacity = new_capacity;
    return true;
}

static void irParameterFree(IRParameter *parameter) {
    if (parameter == NULL) {
        return;
    }

    free(parameter->name);
    irTypeFree(parameter->type);
    parameter->name = NULL;
    parameter->type = NULL;
}

IRBasicBlock *irBasicBlockCreate(unsigned int id) {
    IRBasicBlock *block = calloc(1, sizeof(IRBasicBlock));

    if (block == NULL) {
        return NULL;
    }

    block->id = id;
    return block;
}

bool irBasicBlockAppendInstruction(IRBasicBlock *block, IRInstruction *instruction) {
    if (block == NULL || instruction == NULL) {
        return false;
    }

    if (block->instruction_count == block->instruction_capacity) {
        if (!irGrowPointerArray((void ***) &block->instructions, &block->instruction_capacity, sizeof(IRInstruction *))) {
            return false;
        }
    }

    block->instructions[block->instruction_count++] = instruction;
    return true;
}

void irBasicBlockFree(IRBasicBlock *block) {
    size_t i;

    if (block == NULL) {
        return;
    }

    for (i = 0; i < block->instruction_count; i++) {
        irInstructionFree(block->instructions[i]);
    }

    free(block->instructions);
    free(block);
}

IRFunction *irFunctionCreate(const char *name, IRType *return_type, IRScope *parent_scope) {
    IRFunction *function = calloc(1, sizeof(IRFunction));

    if (function == NULL) {
        irTypeFree(return_type);
        return NULL;
    }

    function->name = irCopyString(name);
    if (name != NULL && function->name == NULL) {
        irTypeFree(return_type);
        free(function);
        return NULL;
    }

    function->return_type = return_type;
    function->scope = irScopeCreate(parent_scope);
    if (function->scope == NULL) {
        irFunctionFree(function);
        return NULL;
    }

    function->next_local_id = 1;
    function->next_block_id = 1;
    return function;
}

unsigned int irFunctionReserveLocalId(IRFunction *function) {
    unsigned int local_id;

    if (function == NULL) {
        return 0;
    }

    local_id = function->next_local_id;
    function->next_local_id++;
    return local_id;
}

unsigned int irFunctionReserveBlockId(IRFunction *function) {
    unsigned int block_id;

    if (function == NULL) {
        return 0;
    }

    block_id = function->next_block_id;
    function->next_block_id++;
    return block_id;
}

IRBasicBlock *irFunctionCreateBlock(IRFunction *function) {
    if (function == NULL) {
        return NULL;
    }

    return irBasicBlockCreate(irFunctionReserveBlockId(function));
}

bool irFunctionAppendBlock(IRFunction *function, IRBasicBlock *block) {
    if (function == NULL || block == NULL) {
        return false;
    }

    if (function->block_count == function->block_capacity) {
        if (!irGrowPointerArray((void ***) &function->blocks, &function->block_capacity, sizeof(IRBasicBlock *))) {
            return false;
        }
    }

    function->blocks[function->block_count++] = block;
    return true;
}

void irFunctionFree(IRFunction *function) {
    size_t i;

    if (function == NULL) {
        return;
    }

    free(function->name);
    irTypeFree(function->return_type);
    for (i = 0; i < function->param_count; i++) {
        irParameterFree(&function->params[i]);
    }
    free(function->params);
    for (i = 0; i < function->block_count; i++) {
        irBasicBlockFree(function->blocks[i]);
    }
    free(function->blocks);
    irScopeFree(function->scope);
    free(function);
}

IRGlobal *irGlobalCreate(const char *name, unsigned int id, IRType *type, IRLiteral *initializer) {
    IRGlobal *global = calloc(1, sizeof(IRGlobal));

    if (global == NULL) {
        irTypeFree(type);
        irLiteralFree(initializer);
        return NULL;
    }

    global->name = irCopyString(name);
    if (name != NULL && global->name == NULL) {
        irGlobalFree(global);
        irTypeFree(type);
        irLiteralFree(initializer);
        return NULL;
    }

    global->id = id;
    global->type = type;
    global->initializer = initializer;
    return global;
}

void irGlobalFree(IRGlobal *global) {
    if (global == NULL) {
        return;
    }

    free(global->name);
    irTypeFree(global->type);
    irLiteralFree(global->initializer);
    free(global);
}

IRModule *irModuleCreate(void) {
    IRModule *module = calloc(1, sizeof(IRModule));

    if (module == NULL) {
        return NULL;
    }

    module->global_scope = irScopeCreate(NULL);
    if (module->global_scope == NULL) {
        irModuleFree(module);
        return NULL;
    }

    return module;
}

bool irModuleAppendGlobal(IRModule *module, IRGlobal *global) {
    if (module == NULL || global == NULL) {
        return false;
    }

    if (module->global_count == module->global_capacity) {
        if (!irGrowPointerArray((void ***) &module->global_items, &module->global_capacity, sizeof(IRGlobal *))) {
            return false;
        }
    }

    module->global_items[module->global_count++] = global;
    return true;
}

bool irModuleAppendFunction(IRModule *module, IRFunction *function) {
    if (module == NULL || function == NULL) {
        return false;
    }

    if (module->function_count == module->function_capacity) {
        if (!irGrowPointerArray((void ***) &module->functions, &module->function_capacity, sizeof(IRFunction *))) {
            return false;
        }
    }

    module->functions[module->function_count++] = function;
    return true;
}

void irModuleFree(IRModule *module) {
    size_t i;

    if (module == NULL) {
        return;
    }

    for (i = 0; i < module->global_count; i++) {
        irGlobalFree(module->global_items[i]);
    }
    for (i = 0; i < module->function_count; i++) {
        irFunctionFree(module->functions[i]);
    }

    free(module->global_items);
    free(module->functions);
    irScopeFree(module->global_scope);
    free(module);
}

IRBuilder *irBuilderCreate(AstNode *program, const SemanticResult *semantic) {
    IRBuilder *builder = calloc(1, sizeof(IRBuilder));

    if (builder == NULL) {
        return NULL;
    }

    builder->program = program;
    builder->semantic = semantic;
    builder->module = irModuleCreate();
    if (builder->module == NULL) {
        free(builder);
        return NULL;
    }

    return builder;
}

void irBuilderFree(IRBuilder *builder) {
    if (builder == NULL) {
        return;
    }

    irModuleFree(builder->module);
    free(builder);
}
