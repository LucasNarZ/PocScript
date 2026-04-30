#include "ir_builder_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IRLiteral *irBuilderLiteralFromAst(const AstNode *node) {
    if (node == NULL) {
        return NULL;
    }

    switch (node->type) {
        case AST_INT_LITERAL:
            return irLiteralCreateInt(node->data.int_literal.value);
        case AST_FLOAT_LITERAL:
            return irLiteralCreateFloat(node->data.float_literal.value);
        case AST_STRING_LITERAL: {
            char *unquoted = irBuilderUnquoteString(node->data.string_literal.value);
            IRLiteral *literal;

            if (unquoted == NULL) {
                return NULL;
            }

            literal = irLiteralCreateString(unquoted);
            free(unquoted);
            return literal;
        }
        case AST_BOOL_LITERAL:
            return irLiteralCreateBool(node->data.bool_literal.value);
        default:
            return NULL;
    }
}

static unsigned int irBuilderNextGlobalId(IRBuilder *builder) {
    return (unsigned int) builder->module->global_count + 1;
}

unsigned int irBuilderAddStringStorage(IRBuilder *builder, const char *value) {
    IRType *char_type;
    IRType *array_type;
    IRLiteral *literal;
    IRGlobal *global;
    unsigned int global_id;
    char name[32];
    char *unquoted;

    global_id = irBuilderNextGlobalId(builder);
    snprintf(name, sizeof(name), ".str.%u", global_id);

    unquoted = irBuilderUnquoteString(value);
    if (unquoted == NULL) {
        return 0;
    }

    char_type = irTypeCreate(IR_TYPE_CHAR);
    array_type = irTypeCreateSizedArray(char_type, strlen(unquoted) + 1);
    literal = irLiteralCreateString(unquoted);
    free(unquoted);
    global = irGlobalCreate(name, global_id, array_type, literal);
    if (global == NULL) {
        return 0;
    }

    global->is_constant = true;
    if (!irModuleAppendGlobal(builder->module, global)) {
        irGlobalFree(global);
        return 0;
    }

    return global_id;
}

static bool irBuilderAppendGlobalSymbol(IRBuilder *builder, const char *name, IRType *type, unsigned int global_id) {
    IRSymbol *symbol = irSymbolCreateGlobal(name, type, global_id);

    if (symbol == NULL) {
        return false;
    }

    if (!irScopeDeclare(builder->module->global_scope, symbol)) {
        irSymbolFree(symbol);
        return false;
    }

    return true;
}

static bool irBuilderPredeclareGlobal(IRBuilder *builder, const AstNode *node) {
    IRType *type = irBuilderTypeFromAst(node->data.var_decl.declared_type);
    IRLiteral *initializer = irBuilderLiteralFromAst(node->data.var_decl.initializer);
    IRGlobal *global;
    unsigned int global_id;

    if (type == NULL) {
        return false;
    }

    global_id = irBuilderNextGlobalId(builder);
    global = irGlobalCreate(node->data.var_decl.name, global_id, type, initializer);
    if (global == NULL) {
        return false;
    }

    if (node->data.var_decl.initializer != NULL && node->data.var_decl.initializer->type == AST_STRING_LITERAL) {
        global->storage_global_id = irBuilderAddStringStorage(builder, node->data.var_decl.initializer->data.string_literal.value);
        global->has_storage_global = global->storage_global_id != 0;
    }

    if (!irModuleAppendGlobal(builder->module, global)) {
        irGlobalFree(global);
        return false;
    }

    return irBuilderAppendGlobalSymbol(builder, node->data.var_decl.name, irTypeClone(global->type), global_id);
}

static bool irBuilderPopulateFunctionParams(IRFunction *function, const AstNode *node) {
    size_t i;

    if (node->data.func_decl.param_count == 0) {
        return true;
    }

    function->params = calloc(node->data.func_decl.param_count, sizeof(IRParameter));
    if (function->params == NULL) {
        return false;
    }

    function->param_count = node->data.func_decl.param_count;
    for (i = 0; i < function->param_count; i++) {
        function->params[i].name = irBuilderCopyString(node->data.func_decl.params[i]->data.param.name);
        function->params[i].type = irBuilderTypeFromAst(node->data.func_decl.params[i]->data.param.declared_type);
        if (function->params[i].name == NULL || function->params[i].type == NULL) {
            return false;
        }
    }

    return true;
}

static IRType **irBuilderCreateFunctionParamTypes(const AstNode *node) {
    IRType **param_types = NULL;
    size_t i;

    if (node->data.func_decl.param_count > 0) {
        param_types = calloc(node->data.func_decl.param_count, sizeof(IRType *));
        if (param_types == NULL) {
            return NULL;
        }
    }

    for (i = 0; i < node->data.func_decl.param_count; i++) {
        param_types[i] = irBuilderTypeFromAst(node->data.func_decl.params[i]->data.param.declared_type);
        if (param_types[i] == NULL) {
            size_t j;
            for (j = 0; j < i; j++) {
                irTypeFree(param_types[j]);
            }
            free(param_types);
            return NULL;
        }
    }

    return param_types;
}

static void irBuilderFreeFunctionParamTypes(IRType **param_types, size_t param_count) {
    size_t i;

    if (param_types == NULL) {
        return;
    }

    for (i = 0; i < param_count; i++) {
        irTypeFree(param_types[i]);
    }

    free(param_types);
}

static bool irBuilderPredeclareFunction(IRBuilder *builder, const AstNode *node) {
    IRType *return_type = irBuilderTypeFromAst(node->data.func_decl.return_type);
    IRFunction *function;
    IRType **param_types;
    IRSymbol *symbol;

    if (return_type == NULL) {
        return false;
    }

    function = irFunctionCreate(node->data.func_decl.name, return_type, builder->module->global_scope);
    if (function == NULL) {
        return false;
    }

    if (!irBuilderPopulateFunctionParams(function, node)) {
        irFunctionFree(function);
        return false;
    }

    param_types = irBuilderCreateFunctionParamTypes(node);
    if (node->data.func_decl.param_count > 0 && param_types == NULL) {
        irFunctionFree(function);
        return false;
    }

    symbol = irSymbolCreateFunction(function->name, irTypeClone(function->return_type), param_types, function->param_count);
    irBuilderFreeFunctionParamTypes(param_types, function->param_count);

    if (symbol == NULL) {
        irFunctionFree(function);
        return false;
    }

    if (!irScopeDeclare(builder->module->global_scope, symbol)) {
        irSymbolFree(symbol);
        irFunctionFree(function);
        return false;
    }

    if (!irModuleAppendFunction(builder->module, function)) {
        irFunctionFree(function);
        return false;
    }

    return true;
}

static bool irBuilderPredeclareExternFunction(IRBuilder *builder, const AstNode *node) {
    IRType *return_type = irBuilderTypeFromAst(node->data.func_decl.return_type);
    IRType **param_types;
    IRSymbol *symbol;

    if (return_type == NULL) {
        return false;
    }

    param_types = irBuilderCreateFunctionParamTypes(node);
    if (node->data.func_decl.param_count > 0 && param_types == NULL) {
        irTypeFree(return_type);
        return false;
    }

    symbol = irSymbolCreateFunction(node->data.func_decl.name, return_type, param_types, node->data.func_decl.param_count);
    irBuilderFreeFunctionParamTypes(param_types, node->data.func_decl.param_count);
    if (symbol == NULL) {
        return false;
    }

    if (!irScopeDeclare(builder->module->global_scope, symbol)) {
        irSymbolFree(symbol);
        return false;
    }

    return true;
}

static bool irBuilderCollectStringLiterals(IRBuilder *builder, const AstNode *node) {
    size_t i;

    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++) {
                if (!irBuilderCollectStringLiterals(builder, node->data.program.items[i])) {
                    return false;
                }
            }
            return true;
        case AST_BLOCK:
            for (i = 0; i < node->data.block.count; i++) {
                if (!irBuilderCollectStringLiterals(builder, node->data.block.items[i])) {
                    return false;
                }
            }
            return true;
        case AST_VAR_DECL:
            return irBuilderCollectStringLiterals(builder, node->data.var_decl.initializer);
        case AST_FUNC_DECL:
            return irBuilderCollectStringLiterals(builder, node->data.func_decl.body);
        case AST_IF:
            return irBuilderCollectStringLiterals(builder, node->data.if_stmt.condition)
                && irBuilderCollectStringLiterals(builder, node->data.if_stmt.then_branch)
                && irBuilderCollectStringLiterals(builder, node->data.if_stmt.else_branch);
        case AST_WHILE:
            return irBuilderCollectStringLiterals(builder, node->data.while_stmt.condition)
                && irBuilderCollectStringLiterals(builder, node->data.while_stmt.body);
        case AST_FOR:
            return irBuilderCollectStringLiterals(builder, node->data.for_stmt.init)
                && irBuilderCollectStringLiterals(builder, node->data.for_stmt.condition)
                && irBuilderCollectStringLiterals(builder, node->data.for_stmt.update)
                && irBuilderCollectStringLiterals(builder, node->data.for_stmt.body);
        case AST_RETURN:
            return irBuilderCollectStringLiterals(builder, node->data.return_stmt.value);
        case AST_EXPR_STMT:
            return irBuilderCollectStringLiterals(builder, node->data.expr_stmt.expression);
        case AST_ASSIGN:
            return irBuilderCollectStringLiterals(builder, node->data.assign.target)
                && irBuilderCollectStringLiterals(builder, node->data.assign.value);
        case AST_BINARY:
            return irBuilderCollectStringLiterals(builder, node->data.binary.left)
                && irBuilderCollectStringLiterals(builder, node->data.binary.right);
        case AST_UNARY:
            return irBuilderCollectStringLiterals(builder, node->data.unary.operand);
        case AST_CALL:
            if (!irBuilderCollectStringLiterals(builder, node->data.call.callee)) {
                return false;
            }
            for (i = 0; i < node->data.call.arg_count; i++) {
                if (!irBuilderCollectStringLiterals(builder, node->data.call.args[i])) {
                    return false;
                }
            }
            return true;
        case AST_ARRAY_ACCESS:
            if (!irBuilderCollectStringLiterals(builder, node->data.array_access.base)) {
                return false;
            }
            for (i = 0; i < node->data.array_access.index_count; i++) {
                if (!irBuilderCollectStringLiterals(builder, node->data.array_access.indices[i])) {
                    return false;
                }
            }
            return true;
        case AST_ARRAY_LITERAL:
            for (i = 0; i < node->data.array_literal.count; i++) {
                if (!irBuilderCollectStringLiterals(builder, node->data.array_literal.elements[i])) {
                    return false;
                }
            }
            return true;
        case AST_STRING_LITERAL:
            return irBuilderAddStringStorage(builder, node->data.string_literal.value) != 0;
        default:
            return true;
    }
}

bool irBuilderMaterializeParameter(IRBuilder *builder, IRFunction *function, const IRParameter *parameter, size_t index) {
    IRType *value_type = irTypeClone(parameter->type);
    IRType *pointer_type = irTypeCreatePointer(irTypeClone(parameter->type));
    IRInstruction *alloca_instr;
    IRInstruction *store_instr;
    IRSymbol *symbol;
    unsigned int slot_id;

    if (value_type == NULL || pointer_type == NULL) {
        irTypeFree(value_type);
        irTypeFree(pointer_type);
        return false;
    }

    slot_id = irBuilderReserveResult(builder);
    alloca_instr = irInstructionCreate(IR_INSTR_ALLOCA);
    store_instr = irInstructionCreate(IR_INSTR_STORE);
    if (alloca_instr == NULL || store_instr == NULL) {
        irInstructionFree(alloca_instr);
        irInstructionFree(store_instr);
        irTypeFree(value_type);
        irTypeFree(pointer_type);
        return false;
    }

    alloca_instr->has_result = true;
    alloca_instr->result_id = slot_id;
    alloca_instr->result_type = pointer_type;
    alloca_instr->data.alloca.allocated_type = irTypeClone(parameter->type);

    store_instr->data.store.address = irOperandCreateLocal(irTypeClone(alloca_instr->result_type), slot_id);
    store_instr->data.store.value = irOperandCreateParam(value_type, (unsigned int) index + 1);

    symbol = irSymbolCreateLocal(parameter->name, irTypeClone(alloca_instr->result_type), slot_id);
    if (symbol == NULL || !irScopeDeclare(function->scope, symbol)) {
        irInstructionFree(alloca_instr);
        irInstructionFree(store_instr);
        if (symbol != NULL) {
            irSymbolFree(symbol);
        }
        return false;
    }

    return irBuilderAppendInstruction(builder, alloca_instr) && irBuilderAppendInstruction(builder, store_instr);
}

static bool irBuilderEmitDefaultReturn(IRBuilder *builder) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_RET);

    if (instruction == NULL) {
        return false;
    }

    if (builder->current_function->return_type != NULL && builder->current_function->return_type->kind != IR_TYPE_VOID) {
        instruction->data.ret.has_value = true;
        switch (builder->current_function->return_type->kind) {
            case IR_TYPE_INT:
            case IR_TYPE_CHAR:
                instruction->data.ret.value = irOperandCreateLiteral(irTypeCreate(builder->current_function->return_type->kind), irLiteralCreateInt(0));
                break;
            case IR_TYPE_BOOL:
                instruction->data.ret.value = irOperandCreateLiteral(irTypeCreate(IR_TYPE_BOOL), irLiteralCreateBool(false));
                break;
            case IR_TYPE_FLOAT:
                instruction->data.ret.value = irOperandCreateLiteral(irTypeCreate(IR_TYPE_FLOAT), irLiteralCreateFloat(0.0));
                break;
            default:
                instruction->data.ret.value = irOperandCreateLiteral(irTypeClone(builder->current_function->return_type), NULL);
                break;
        }
    }

    return irBuilderAppendInstruction(builder, instruction);
}

static bool irBuilderLowerFunctionBody(IRBuilder *builder, const AstNode *node, IRFunction *function) {
    size_t i;
    IRBasicBlock *entry_block;

    builder->current_function = function;
    builder->current_scope = function->scope;
    entry_block = irFunctionCreateBlock(function);
    if (entry_block == NULL || !irFunctionAppendBlock(function, entry_block)) {
        irBasicBlockFree(entry_block);
        return false;
    }
    builder->current_block = entry_block;

    for (i = 0; i < function->param_count; i++) {
        if (!irBuilderMaterializeParameter(builder, function, &function->params[i], i)) {
            return false;
        }
    }

    for (i = 0; i < node->data.func_decl.body->data.block.count; i++) {
        if (!irBuilderLowerStatement(builder, node->data.func_decl.body->data.block.items[i])) {
            return false;
        }
    }

    if (!irBuilderCurrentBlockHasTerminator(builder)) {
        return irBuilderEmitDefaultReturn(builder);
    }

    return true;
}

IRModule *irBuildModule(AstNode *program, const SemanticResult *semantic) {
    IRBuilder *builder;
    IRModule *module;
    size_t i;
    size_t function_index;

    if (program == NULL || semantic == NULL || semantic->errors.count != 0) {
        return NULL;
    }

    builder = irBuilderCreate(program, semantic);
    if (builder == NULL) {
        return NULL;
    }

    for (i = 0; i < program->data.program.count; i++) {
        AstNode *node = program->data.program.items[i];

        if (node->type == AST_VAR_DECL) {
            if (!irBuilderPredeclareGlobal(builder, node)) {
                irBuilderFree(builder);
                return NULL;
            }
        } else if (node->type == AST_FUNC_DECL) {
            if (node->data.func_decl.is_extern) {
                if (!irBuilderPredeclareExternFunction(builder, node)) {
                    irBuilderFree(builder);
                    return NULL;
                }
            } else if (!irBuilderPredeclareFunction(builder, node)) {
                irBuilderFree(builder);
                return NULL;
            }
        }
    }

    if (!irBuilderCollectStringLiterals(builder, program)) {
        irBuilderFree(builder);
        return NULL;
    }

    function_index = 0;
    for (i = 0; i < program->data.program.count; i++) {
        AstNode *node = program->data.program.items[i];

        if (node->type != AST_FUNC_DECL) {
            continue;
        }

        if (node->data.func_decl.is_extern) {
            continue;
        }

        if (!irBuilderLowerFunctionBody(builder, node, builder->module->functions[function_index])) {
            irBuilderFree(builder);
            return NULL;
        }
        function_index++;
    }

    module = builder->module;
    builder->module = NULL;
    irBuilderFree(builder);

    return module;
}
