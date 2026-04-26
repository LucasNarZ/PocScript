#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *irBuilderCopyString(const char *value) {
    size_t length;
    char *copy;

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

static char *irBuilderUnquoteString(const char *value) {
    size_t length;
    char *copy;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value);
    if (length >= 2 && ((value[0] == '"' && value[length - 1] == '"') || (value[0] == '\'' && value[length - 1] == '\''))) {
        copy = malloc(length - 1);
        if (copy == NULL) {
            return NULL;
        }

        memcpy(copy, value + 1, length - 2);
        copy[length - 2] = '\0';
        return copy;
    }

    return irBuilderCopyString(value);
}

static IRType *irBuilderTypeFromAst(const AstNode *type_node) {
    IRType *element_type;
    const AstNode *element_type_node;

    if (type_node == NULL) {
        return NULL;
    }

    if (type_node->type == AST_TYPE_ARRAY) {
        element_type_node = type_node->data.type_array.element_type;
        if (type_node->data.type_array.size_expr != NULL
                && element_type_node != NULL
                && element_type_node->type == AST_TYPE_ARRAY
                && element_type_node->data.type_array.size_expr == NULL) {
            element_type_node = element_type_node->data.type_array.element_type;
        }

        element_type = irBuilderTypeFromAst(element_type_node);
        if (type_node->data.type_array.size_expr != NULL && type_node->data.type_array.size_expr->type == AST_INT_LITERAL) {
            return irTypeCreateSizedArray(element_type, (size_t) type_node->data.type_array.size_expr->data.int_literal.value);
        }
        return irTypeCreateArray(element_type);
    }

    if (type_node->type != AST_TYPE_NAME) {
        return NULL;
    }

    switch (type_node->data.type_name.kind) {
        case AST_TYPE_INT_KIND:
            return irTypeCreate(IR_TYPE_INT);
        case AST_TYPE_FLOAT_KIND:
            return irTypeCreate(IR_TYPE_FLOAT);
        case AST_TYPE_CHAR_KIND:
            return irTypeCreate(IR_TYPE_CHAR);
        case AST_TYPE_BOOL_KIND:
            return irTypeCreate(IR_TYPE_BOOL);
        case AST_TYPE_VOID_KIND:
            return irTypeCreate(IR_TYPE_VOID);
        case AST_TYPE_CUSTOM_KIND:
            if (type_node->data.type_name.name != NULL && strcmp(type_node->data.type_name.name, "string") == 0) {
                return irTypeCreate(IR_TYPE_STRING);
            }
            return NULL;
        default:
            return NULL;
    }
}

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

static IRLiteral *irBuilderCloneLiteral(const IRLiteral *literal) {
    if (literal == NULL) {
        return NULL;
    }

    switch (literal->kind) {
        case IR_LITERAL_INT:
            return irLiteralCreateInt(literal->data.int_value);
        case IR_LITERAL_FLOAT:
            return irLiteralCreateFloat(literal->data.float_value);
        case IR_LITERAL_STRING:
            return irLiteralCreateString(literal->data.string_value);
        case IR_LITERAL_BOOL:
            return irLiteralCreateBool(literal->data.bool_value);
    }

    return NULL;
}

static IROperand irBuilderCloneOperand(const IROperand *operand) {
    if (operand == NULL) {
        return (IROperand) {0};
    }

    switch (operand->kind) {
        case IR_OPERAND_LOCAL:
            return irOperandCreateLocal(irTypeClone(operand->type), operand->data.local_id);
        case IR_OPERAND_PARAM:
            return irOperandCreateParam(irTypeClone(operand->type), operand->data.param_id);
        case IR_OPERAND_GLOBAL:
            return irOperandCreateGlobal(irTypeClone(operand->type), operand->data.global_id);
        case IR_OPERAND_LITERAL:
            return irOperandCreateLiteral(irTypeClone(operand->type), irBuilderCloneLiteral(operand->data.literal));
        case IR_OPERAND_BLOCK:
            return irOperandCreateBlock(operand->data.block_id);
    }

    return (IROperand) {0};
}

static void irBuilderFreeOperandArray(IROperand *operands, size_t count) {
    size_t i;

    if (operands == NULL) {
        return;
    }

    for (i = 0; i < count; i++) {
        irOperandFree(&operands[i]);
    }

    free(operands);
}

static size_t irBuilderArrayDepth(const IRType *type) {
    size_t depth = 0;
    const IRType *current = type;

    while (current != NULL && current->kind == IR_TYPE_ARRAY) {
        depth++;
        current = current->element_type;
    }

    return depth;
}

static IRType *irBuilderInferArrayTypeFromLiteral(const IRType *declared_type, const AstNode *literal) {
    IRType *element_type;

    if (declared_type == NULL) {
        return NULL;
    }

    if (declared_type->kind != IR_TYPE_ARRAY || literal == NULL || literal->type != AST_ARRAY_LITERAL) {
        return irTypeClone(declared_type);
    }

    if (declared_type->element_type != NULL
            && declared_type->element_type->kind == IR_TYPE_ARRAY
            && literal->data.array_literal.count > 0
            && literal->data.array_literal.elements[0]->type == AST_ARRAY_LITERAL) {
        element_type = irBuilderInferArrayTypeFromLiteral(declared_type->element_type, literal->data.array_literal.elements[0]);
    } else {
        element_type = irTypeClone(declared_type->element_type);
    }

    if (element_type == NULL) {
        return NULL;
    }

    return irTypeCreateSizedArray(element_type, literal->data.array_literal.count);
}

static bool irBuilderPushLoopTarget(IRBuilder *builder, IRBasicBlock *break_target, IRBasicBlock *continue_target) {
    IRLoopTarget *resized;

    if (builder->loop_target_count == builder->loop_target_capacity) {
        size_t new_capacity = builder->loop_target_capacity == 0 ? 4 : builder->loop_target_capacity * 2;
        resized = realloc(builder->loop_targets, new_capacity * sizeof(IRLoopTarget));
        if (resized == NULL) {
            return false;
        }

        builder->loop_targets = resized;
        builder->loop_target_capacity = new_capacity;
    }

    builder->loop_targets[builder->loop_target_count].break_target = break_target;
    builder->loop_targets[builder->loop_target_count].continue_target = continue_target;
    builder->loop_target_count++;
    return true;
}

static void irBuilderPopLoopTarget(IRBuilder *builder) {
    if (builder->loop_target_count > 0) {
        builder->loop_target_count--;
    }
}

static IRLoopTarget *irBuilderCurrentLoopTarget(IRBuilder *builder) {
    if (builder->loop_target_count == 0) {
        return NULL;
    }

    return &builder->loop_targets[builder->loop_target_count - 1];
}

static unsigned int irBuilderNextGlobalId(IRBuilder *builder) {
    return (unsigned int) builder->module->global_count + 1;
}

static bool irBuilderAppendInstruction(IRBuilder *builder, IRInstruction *instruction) {
    if (instruction == NULL) {
        return false;
    }

    if (!irBasicBlockAppendInstruction(builder->current_block, instruction)) {
        irInstructionFree(instruction);
        return false;
    }

    return true;
}

static bool irBuilderCurrentBlockHasTerminator(IRBuilder *builder) {
    IRInstruction *instruction;

    if (builder == NULL || builder->current_block == NULL || builder->current_block->instruction_count == 0) {
        return false;
    }

    instruction = builder->current_block->instructions[builder->current_block->instruction_count - 1];
    return instruction->kind == IR_INSTR_RET || instruction->kind == IR_INSTR_BR || instruction->kind == IR_INSTR_COND_BR;
}

static unsigned int irBuilderReserveResult(IRBuilder *builder) {
    return irFunctionReserveLocalId(builder->current_function);
}

static IRSymbol *irBuilderLookupLocal(IRBuilder *builder, const char *name) {
    return irScopeLookup(builder->current_scope, name);
}

static IRValue irBuilderLowerRValue(IRBuilder *builder, const AstNode *node);
static IRValue irBuilderLowerLValue(IRBuilder *builder, const AstNode *node);

static size_t irBuilderStringLiteralLength(const char *value) {
    size_t length;

    if (value == NULL) {
        return 0;
    }

    length = strlen(value);
    if (length >= 2 && ((value[0] == '"' && value[length - 1] == '"') || (value[0] == '\'' && value[length - 1] == '\''))) {
        return length - 2;
    }

    return length;
}

static unsigned int irBuilderAddStringStorage(IRBuilder *builder, const char *value) {
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

static bool irBuilderPredeclareFunction(IRBuilder *builder, const AstNode *node) {
    IRType *return_type = irBuilderTypeFromAst(node->data.func_decl.return_type);
    IRFunction *function;
    IRType **param_types = NULL;
    IRSymbol *symbol;
    size_t i;

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

    if (function->param_count > 0) {
        param_types = calloc(function->param_count, sizeof(IRType *));
        if (param_types == NULL) {
            irFunctionFree(function);
            return false;
        }
    }

    for (i = 0; i < function->param_count; i++) {
        param_types[i] = irTypeClone(function->params[i].type);
    }

    symbol = irSymbolCreateFunction(function->name, irTypeClone(function->return_type), param_types, function->param_count);
    for (i = 0; i < function->param_count; i++) {
        irTypeFree(param_types[i]);
    }
    free(param_types);

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

static bool irBuilderDeclareBuiltinFunction(IRBuilder *builder, const char *name, IRType *return_type, IRType **param_types, size_t param_count) {
    IRSymbol *symbol = irSymbolCreateFunction(name, return_type, param_types, param_count);

    if (symbol == NULL) {
        return false;
    }

    if (!irScopeDeclare(builder->module->global_scope, symbol)) {
        irSymbolFree(symbol);
        return false;
    }

    return true;
}

static bool irBuilderDeclareBuiltinRuntimeFunctions(IRBuilder *builder) {
    IRType *print_string_params[1];
    IRType *print_int_params[1];

    print_string_params[0] = irTypeCreate(IR_TYPE_STRING);
    if (print_string_params[0] == NULL) {
        return false;
    }
    if (!irBuilderDeclareBuiltinFunction(builder, "printString", irTypeCreate(IR_TYPE_VOID), print_string_params, 1)) {
        irTypeFree(print_string_params[0]);
        return false;
    }
    irTypeFree(print_string_params[0]);

    print_int_params[0] = irTypeCreate(IR_TYPE_INT);
    if (print_int_params[0] == NULL) {
        return false;
    }
    if (!irBuilderDeclareBuiltinFunction(builder, "printInt", irTypeCreate(IR_TYPE_VOID), print_int_params, 1)) {
        irTypeFree(print_int_params[0]);
        return false;
    }
    irTypeFree(print_int_params[0]);

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

static bool irBuilderMaterializeParameter(IRBuilder *builder, IRFunction *function, const IRParameter *parameter, size_t index) {
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

static IRValue irBuilderLiteralValue(IRTypeKind kind, IRLiteral *literal) {
    IRValue result = irValueEmpty();
    IRType *type = irTypeCreate(kind);

    if (type == NULL || literal == NULL) {
        irTypeFree(type);
        irLiteralFree(literal);
        return result;
    }

    result.type = irTypeClone(type);
    result.value = irOperandCreateLiteral(type, literal);
    result.has_value = true;
    return result;
}

static IRValue irBuilderEmitLoad(IRBuilder *builder, IROperand address, IRType *value_type) {
    IRValue result = irValueEmpty();
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_LOAD);
    unsigned int result_id;

    if (instruction == NULL || value_type == NULL) {
        irInstructionFree(instruction);
        irTypeFree(value_type);
        return result;
    }

    result_id = irBuilderReserveResult(builder);
    instruction->has_result = true;
    instruction->result_id = result_id;
    instruction->result_type = irTypeClone(value_type);
    instruction->data.load.address = address;

    if (!irBuilderAppendInstruction(builder, instruction)) {
        irTypeFree(value_type);
        return result;
    }

    result.type = value_type;
    result.value = irOperandCreateLocal(irTypeClone(value_type), result_id);
    result.has_value = true;
    return result;
}

static IRValue irBuilderEmitBinary(IRBuilder *builder, IRInstructionKind kind, IROperand left, IROperand right, IRType *type) {
    IRValue result = irValueEmpty();
    IRInstruction *instruction = irInstructionCreate(kind);
    unsigned int result_id;

    if (instruction == NULL || type == NULL) {
        irInstructionFree(instruction);
        irOperandFree(&left);
        irOperandFree(&right);
        irTypeFree(type);
        return result;
    }

    result_id = irBuilderReserveResult(builder);
    instruction->has_result = true;
    instruction->result_id = result_id;
    instruction->result_type = irTypeClone(type);
    instruction->data.binary.left = left;
    instruction->data.binary.right = right;

    if (!irBuilderAppendInstruction(builder, instruction)) {
        irTypeFree(type);
        return result;
    }

    result.type = type;
    result.value = irOperandCreateLocal(irTypeClone(type), result_id);
    result.has_value = true;
    return result;
}

static IRValue irBuilderEmitUnary(IRBuilder *builder, IRInstructionKind kind, IROperand operand, IRType *type) {
    IRValue result = irValueEmpty();
    IRInstruction *instruction = irInstructionCreate(kind);
    unsigned int result_id;

    if (instruction == NULL || type == NULL) {
        irInstructionFree(instruction);
        irOperandFree(&operand);
        irTypeFree(type);
        return result;
    }

    result_id = irBuilderReserveResult(builder);
    instruction->has_result = true;
    instruction->result_id = result_id;
    instruction->result_type = irTypeClone(type);
    instruction->data.unary.operand = operand;

    if (!irBuilderAppendInstruction(builder, instruction)) {
        irTypeFree(type);
        return result;
    }

    result.type = type;
    result.value = irOperandCreateLocal(irTypeClone(type), result_id);
    result.has_value = true;
    return result;
}

static IRValue irBuilderEmitGep(IRBuilder *builder, IROperand base, IROperand *indices, size_t index_count, IRType *result_type) {
    IRValue result = irValueEmpty();
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_GEP);
    unsigned int result_id;

    if (instruction == NULL || result_type == NULL) {
        irInstructionFree(instruction);
        irOperandFree(&base);
        free(indices);
        irTypeFree(result_type);
        return result;
    }

    result_id = irBuilderReserveResult(builder);
    instruction->has_result = true;
    instruction->result_id = result_id;
    instruction->result_type = irTypeCreatePointer(irTypeClone(result_type));
    instruction->data.gep.base = base;
    instruction->data.gep.indices = indices;
    instruction->data.gep.index_count = index_count;

    if (!irBuilderAppendInstruction(builder, instruction)) {
        irTypeFree(result_type);
        return result;
    }

    result.type = result_type;
    result.address = irOperandCreateLocal(irTypeClone(instruction->result_type), result_id);
    result.has_address = true;
    return result;
}

static bool irBuilderEmitStore(IRBuilder *builder, IROperand address, IROperand value);

static bool irBuilderStoreArrayLiteralElements(
        IRBuilder *builder,
        const IROperand *base_address,
        const IRType *array_type,
        const AstNode *literal,
        long *path,
        size_t depth) {
    size_t i;

    if (base_address == NULL || array_type == NULL || literal == NULL || literal->type != AST_ARRAY_LITERAL) {
        return false;
    }

    for (i = 0; i < literal->data.array_literal.count; i++) {
        AstNode *element = literal->data.array_literal.elements[i];
        path[depth] = (long) i;

        if (element->type == AST_ARRAY_LITERAL) {
            if (array_type->element_type == NULL || array_type->element_type->kind != IR_TYPE_ARRAY) {
                return false;
            }

            if (!irBuilderStoreArrayLiteralElements(builder, base_address, array_type->element_type, element, path, depth + 1)) {
                return false;
            }

            continue;
        }

        {
            IROperand *indices = calloc(depth + 2, sizeof(IROperand));
            IRValue slot;
            IRValue initializer;
            size_t j;

            if (indices == NULL) {
                return false;
            }

            indices[0] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));
            for (j = 0; j <= depth; j++) {
                indices[j + 1] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(path[j]));
            }

            slot = irBuilderEmitGep(
                builder,
                irBuilderCloneOperand(base_address),
                indices,
                depth + 2,
                irTypeClone(array_type->element_type)
            );
            initializer = irBuilderLowerRValue(builder, element);
            if (!slot.has_address || !initializer.has_value) {
                irTypeFree(slot.type);
                irTypeFree(initializer.type);
                return false;
            }

            if (!irBuilderEmitStore(builder, slot.address, initializer.value)) {
                irTypeFree(slot.type);
                irTypeFree(initializer.type);
                return false;
            }

            irTypeFree(slot.type);
            irTypeFree(initializer.type);
        }
    }

    return true;
}

static bool irBuilderEmitBranch(IRBuilder *builder, IRBasicBlock *target) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_BR);

    if (instruction == NULL) {
        return false;
    }

    instruction->data.br.target_block_id = target->id;
    return irBuilderAppendInstruction(builder, instruction);
}

static bool irBuilderEmitCondBranch(IRBuilder *builder, IROperand condition, IRBasicBlock *then_block, IRBasicBlock *else_block) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_COND_BR);

    if (instruction == NULL) {
        irOperandFree(&condition);
        return false;
    }

    instruction->data.cond_br.condition = condition;
    instruction->data.cond_br.then_block_id = then_block->id;
    instruction->data.cond_br.else_block_id = else_block->id;
    return irBuilderAppendInstruction(builder, instruction);
}

static bool irBuilderEmitStore(IRBuilder *builder, IROperand address, IROperand value) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_STORE);

    if (instruction == NULL) {
        irOperandFree(&address);
        irOperandFree(&value);
        return false;
    }

    instruction->data.store.address = address;
    instruction->data.store.value = value;
    return irBuilderAppendInstruction(builder, instruction);
}

static IRValue irBuilderLowerIdentifier(IRBuilder *builder, const AstNode *node) {
    IRSymbol *symbol = irBuilderLookupLocal(builder, node->data.identifier.name);
    IRValue result = irValueEmpty();

    if (symbol == NULL || symbol->type == NULL) {
        return result;
    }

    if (symbol->kind == IR_SYMBOL_LOCAL && symbol->type->kind == IR_TYPE_POINTER) {
        result.type = irTypeClone(symbol->type->element_type);
        result.address = irOperandCreateLocal(irTypeClone(symbol->type), symbol->data.local_id);
        result.has_address = true;
        return result;
    }

    if (symbol->kind == IR_SYMBOL_GLOBAL) {
        IRType *value_type = irTypeClone(symbol->type);
        IRType *pointer_type;

        if (value_type == NULL) {
            return result;
        }

        pointer_type = irTypeCreatePointer(irTypeClone(symbol->type));
        if (pointer_type == NULL) {
            irTypeFree(value_type);
            return result;
        }

        result.type = value_type;
        result.address = irOperandCreateGlobal(pointer_type, symbol->data.global_id);
        result.has_address = true;
        return result;
    }

    return result;
}

static IRValue irBuilderLowerArrayAccess(IRBuilder *builder, const AstNode *node) {
    IRValue base = irBuilderLowerLValue(builder, node->data.array_access.base);
    IROperand *indices;
    IRType *current_type;
    size_t i;

    if (!base.has_address || base.type == NULL || base.type->kind != IR_TYPE_ARRAY || node->data.array_access.index_count == 0) {
        irTypeFree(base.type);
        if (base.has_address) {
            irOperandFree(&base.address);
        }
        return irValueEmpty();
    }

    indices = calloc(node->data.array_access.index_count + 1, sizeof(IROperand));
    if (indices == NULL) {
        irTypeFree(base.type);
        irOperandFree(&base.address);
        return irValueEmpty();
    }

    current_type = irTypeClone(base.type);
    indices[0] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));

    for (i = 0; i < node->data.array_access.index_count; i++) {
        IRValue index_value;
        IRType *next_type;

        if (current_type == NULL || current_type->kind != IR_TYPE_ARRAY || current_type->element_type == NULL) {
            irTypeFree(base.type);
            irTypeFree(current_type);
            irOperandFree(&base.address);
            irBuilderFreeOperandArray(indices, i + 1);
            return irValueEmpty();
        }

        index_value = irBuilderLowerRValue(builder, node->data.array_access.indices[i]);
        if (!index_value.has_value) {
            irTypeFree(base.type);
            irTypeFree(current_type);
            irOperandFree(&base.address);
            irBuilderFreeOperandArray(indices, i + 1);
            return irValueEmpty();
        }

        indices[i + 1] = index_value.value;
        irTypeFree(index_value.type);
        next_type = irTypeClone(current_type->element_type);
        irTypeFree(current_type);
        current_type = next_type;
    }

    irTypeFree(base.type);
    return irBuilderEmitGep(builder, base.address, indices, node->data.array_access.index_count + 1, current_type);
}

static IRValue irBuilderLowerLValue(IRBuilder *builder, const AstNode *node) {
    if (node == NULL) {
        return irValueEmpty();
    }

    if (node->type == AST_IDENTIFIER) {
        return irBuilderLowerIdentifier(builder, node);
    }

    if (node->type == AST_ARRAY_ACCESS) {
        return irBuilderLowerArrayAccess(builder, node);
    }

    return irValueEmpty();
}

static IRValue irBuilderLowerRValue(IRBuilder *builder, const AstNode *node) {
    IRValue result = irValueEmpty();
    IRValue left;
    IRValue right;
    IRValue binary_result;

    if (node == NULL) {
        return result;
    }

    switch (node->type) {
        case AST_INT_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_INT, irLiteralCreateInt(node->data.int_literal.value));
        case AST_FLOAT_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_FLOAT, irLiteralCreateFloat(node->data.float_literal.value));
        case AST_BOOL_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_BOOL, irLiteralCreateBool(node->data.bool_literal.value));
        case AST_STRING_LITERAL: {
            unsigned int global_id = irBuilderAddStringStorage(builder, node->data.string_literal.value);
            IRType *char_type;
            IRType *array_type;
            IRType *pointer_type;
            IROperand base;
            IROperand *indices;
            IRValue address_value;

            if (global_id == 0) {
                return irValueEmpty();
            }

            char_type = irTypeCreate(IR_TYPE_CHAR);
            array_type = irTypeCreateSizedArray(char_type, irBuilderStringLiteralLength(node->data.string_literal.value) + 1);
            pointer_type = irTypeCreatePointer(irTypeClone(array_type));
            if (array_type == NULL || pointer_type == NULL) {
                irTypeFree(array_type);
                irTypeFree(pointer_type);
                return irValueEmpty();
            }

            indices = calloc(2, sizeof(IROperand));
            if (indices == NULL) {
                irTypeFree(array_type);
                irTypeFree(pointer_type);
                return irValueEmpty();
            }

            base = irOperandCreateGlobal(pointer_type, global_id);
            indices[0] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));
            indices[1] = irOperandCreateLiteral(irTypeCreate(IR_TYPE_INT), irLiteralCreateInt(0));
            address_value = irBuilderEmitGep(builder, base, indices, 2, irTypeCreate(IR_TYPE_CHAR));
            irTypeFree(array_type);

            if (!address_value.has_address) {
                return irValueEmpty();
            }

            result.type = irTypeCreate(IR_TYPE_STRING);
            result.value = address_value.address;
            result.has_value = true;
            return result;
        }
        case AST_IDENTIFIER:
            result = irBuilderLowerIdentifier(builder, node);
            if (!result.has_address || result.type == NULL) {
                return irValueEmpty();
            }
            return irBuilderEmitLoad(builder, result.address, result.type);
        case AST_ARRAY_ACCESS:
            result = irBuilderLowerArrayAccess(builder, node);
            if (!result.has_address || result.type == NULL) {
                return irValueEmpty();
            }
            return irBuilderEmitLoad(builder, result.address, result.type);
        case AST_UNARY: {
            IRValue operand = irBuilderLowerRValue(builder, node->data.unary.operand);
            if (!operand.has_value) {
                return irValueEmpty();
            }

            if (node->data.unary.op == AST_UNARY_NOT) {
                IRValue unary_result = irBuilderEmitUnary(builder, IR_INSTR_NOT, operand.value, irTypeCreate(IR_TYPE_BOOL));
                irTypeFree(operand.type);
                return unary_result;
            }

            if (node->data.unary.op == AST_UNARY_NEGATE) {
                IRValue unary_result = irBuilderEmitUnary(builder, IR_INSTR_NEG, operand.value, irTypeClone(operand.type));
                irTypeFree(operand.type);
                return unary_result;
            }

            irOperandFree(&operand.value);
            irTypeFree(operand.type);
            return irValueEmpty();
        }
        case AST_CALL: {
            IRSymbol *callee;
            IROperand *args = NULL;
            size_t i;
            IRInstruction *instruction;
            unsigned int result_id = 0;

            if (node->data.call.callee->type != AST_IDENTIFIER) {
                return irValueEmpty();
            }

            callee = irScopeLookup(builder->module->global_scope, node->data.call.callee->data.identifier.name);
            if (callee == NULL) {
                return irValueEmpty();
            }

            if (node->data.call.arg_count > 0) {
                args = calloc(node->data.call.arg_count, sizeof(IROperand));
                if (args == NULL) {
                    return irValueEmpty();
                }
            }

            for (i = 0; i < node->data.call.arg_count; i++) {
                IRValue arg = irBuilderLowerRValue(builder, node->data.call.args[i]);
                if (!arg.has_value) {
                    free(args);
                    return irValueEmpty();
                }

                args[i] = arg.value;
                irTypeFree(arg.type);
            }

            instruction = irInstructionCreate(IR_INSTR_CALL);
            if (instruction == NULL) {
                for (i = 0; i < node->data.call.arg_count; i++) {
                    irOperandFree(&args[i]);
                }
                free(args);
                return irValueEmpty();
            }

            instruction->data.call.callee = irBuilderCopyString(node->data.call.callee->data.identifier.name);
            instruction->data.call.args = args;
            instruction->data.call.arg_count = node->data.call.arg_count;
            if (callee->type != NULL && callee->type->kind != IR_TYPE_VOID) {
                result_id = irBuilderReserveResult(builder);
                instruction->has_result = true;
                instruction->result_id = result_id;
                instruction->result_type = irTypeClone(callee->type);
            }

            if (!irBuilderAppendInstruction(builder, instruction)) {
                return irValueEmpty();
            }

            if (callee->type != NULL && callee->type->kind != IR_TYPE_VOID) {
                result.type = irTypeClone(callee->type);
                result.value = irOperandCreateLocal(irTypeClone(callee->type), result_id);
                result.has_value = true;
            }

            return result;
        }
        case AST_BINARY:
            left = irBuilderLowerRValue(builder, node->data.binary.left);
            right = irBuilderLowerRValue(builder, node->data.binary.right);
            if (!left.has_value || !right.has_value || left.type == NULL) {
                irTypeFree(left.type);
                irTypeFree(right.type);
                return irValueEmpty();
            }

            switch (node->data.binary.op) {
                case AST_BINARY_ADD:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_ADD, left.value, right.value, left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_SUB:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_SUB, left.value, right.value, left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_MUL:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_MUL, left.value, right.value, left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_DIV:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_DIV, left.value, right.value, left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_AND:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_AND, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_OR:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_OR, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_GT:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_GT, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_LT:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_LT, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_LTE:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_LE, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_GTE:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_GE, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_EQ:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_EQ, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                case AST_BINARY_NEQ:
                    binary_result = irBuilderEmitBinary(builder, IR_INSTR_CMP_NE, left.value, right.value, irTypeCreate(IR_TYPE_BOOL));
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return binary_result;
                default:
                    irOperandFree(&left.value);
                    irOperandFree(&right.value);
                    irTypeFree(left.type);
                    irTypeFree(right.type);
                    return irValueEmpty();
            }
        default:
            return result;
    }
}

static bool irBuilderLowerVarDecl(IRBuilder *builder, const AstNode *node) {
    IRType *value_type = irBuilderTypeFromAst(node->data.var_decl.declared_type);
    IRType *pointer_type;
    IRInstruction *alloca_instr;
    IRSymbol *symbol;
    unsigned int slot_id;

    if (value_type == NULL) {
        return false;
    }

    if (node->data.var_decl.initializer != NULL
            && value_type->kind == IR_TYPE_ARRAY
            && !value_type->has_array_size
            && node->data.var_decl.initializer->type == AST_ARRAY_LITERAL) {
        IRType *inferred_type = irBuilderInferArrayTypeFromLiteral(value_type, node->data.var_decl.initializer);

        if (inferred_type == NULL) {
            irTypeFree(value_type);
            return false;
        }

        irTypeFree(value_type);
        value_type = inferred_type;
    }

    pointer_type = irTypeCreatePointer(irTypeClone(value_type));
    if (pointer_type == NULL) {
        irTypeFree(value_type);
        return false;
    }

    slot_id = irBuilderReserveResult(builder);
    alloca_instr = irInstructionCreate(IR_INSTR_ALLOCA);
    if (alloca_instr == NULL) {
        irTypeFree(value_type);
        irTypeFree(pointer_type);
        return false;
    }

    alloca_instr->has_result = true;
    alloca_instr->result_id = slot_id;
    alloca_instr->result_type = pointer_type;
    alloca_instr->data.alloca.allocated_type = irTypeClone(value_type);

    symbol = irSymbolCreateLocal(node->data.var_decl.name, irTypeClone(pointer_type), slot_id);
    if (symbol == NULL || !irScopeDeclare(builder->current_scope, symbol)) {
        irInstructionFree(alloca_instr);
        irTypeFree(value_type);
        if (symbol != NULL) {
            irSymbolFree(symbol);
        }
        return false;
    }

    if (!irBuilderAppendInstruction(builder, alloca_instr)) {
        irTypeFree(value_type);
        return false;
    }

    if (node->data.var_decl.initializer != NULL && value_type->kind == IR_TYPE_ARRAY && node->data.var_decl.initializer->type == AST_ARRAY_LITERAL) {
        size_t array_depth = irBuilderArrayDepth(value_type);
        long *path = calloc(array_depth, sizeof(long));
        IROperand base_address = irOperandCreateLocal(irTypeClone(pointer_type), slot_id);

        if (path == NULL) {
            irOperandFree(&base_address);
            irTypeFree(value_type);
            return false;
        }

        if (!irBuilderStoreArrayLiteralElements(builder, &base_address, value_type, node->data.var_decl.initializer, path, 0)) {
            free(path);
            irOperandFree(&base_address);
            irTypeFree(value_type);
            return false;
        }

        free(path);
        irOperandFree(&base_address);
    } else if (node->data.var_decl.initializer != NULL && value_type->kind != IR_TYPE_STRING) {
        IRValue initializer = irBuilderLowerRValue(builder, node->data.var_decl.initializer);

        if (!initializer.has_value) {
            irTypeFree(value_type);
            return false;
        }

        if (!irBuilderEmitStore(
                builder,
                irOperandCreateLocal(irTypeClone(pointer_type), slot_id),
                initializer.value)) {
            irTypeFree(initializer.type);
            irTypeFree(value_type);
            return false;
        }

        irTypeFree(initializer.type);
    }

    irTypeFree(value_type);
    return true;
}

static bool irBuilderLowerAssign(IRBuilder *builder, const AstNode *node) {
    IRValue target = irBuilderLowerLValue(builder, node->data.assign.target);
    IRValue rhs;

    if (!target.has_address || target.type == NULL) {
        return false;
    }

    if (node->data.assign.op == AST_ASSIGN_SET) {
        rhs = irBuilderLowerRValue(builder, node->data.assign.value);
        if (!rhs.has_value) {
            irTypeFree(target.type);
            irOperandFree(&target.address);
            return false;
        }

        irTypeFree(target.type);
        if (!irBuilderEmitStore(builder, target.address, rhs.value)) {
            irTypeFree(rhs.type);
            return false;
        }

        irTypeFree(rhs.type);
        return true;
    }

    {
        IRValue current = irBuilderEmitLoad(builder, target.address, target.type);
        IRValue value = irBuilderLowerRValue(builder, node->data.assign.value);
        IRValue computed;
        IRValue target_again;

        if (!current.has_value || !value.has_value) {
            irTypeFree(value.type);
            return false;
        }

        computed = irBuilderEmitBinary(
            builder,
            node->data.assign.op == AST_ASSIGN_ADD ? IR_INSTR_ADD : IR_INSTR_SUB,
            current.value,
            value.value,
            current.type
        );

        if (!computed.has_value) {
            irTypeFree(value.type);
            return false;
        }

        target_again = irBuilderLowerLValue(builder, node->data.assign.target);
        if (!target_again.has_address) {
            irTypeFree(value.type);
            irTypeFree(computed.type);
            return false;
        }

        if (!irBuilderEmitStore(builder, target_again.address, computed.value)) {
            irTypeFree(target_again.type);
            irTypeFree(value.type);
            irTypeFree(computed.type);
            return false;
        }

        irTypeFree(target_again.type);
        irTypeFree(value.type);
        irTypeFree(computed.type);
        return true;
    }
}

static bool irBuilderLowerReturn(IRBuilder *builder, const AstNode *node) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_RET);

    if (instruction == NULL) {
        return false;
    }

    if (node->data.return_stmt.value != NULL) {
        IRValue value = irBuilderLowerRValue(builder, node->data.return_stmt.value);
        if (!value.has_value) {
            irInstructionFree(instruction);
            return false;
        }

        instruction->data.ret.has_value = true;
        instruction->data.ret.value = value.value;
        irTypeFree(value.type);
    }

    return irBuilderAppendInstruction(builder, instruction);
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

static bool irBuilderLowerStatement(IRBuilder *builder, const AstNode *node);

static bool irBuilderLowerLooseNode(IRBuilder *builder, const AstNode *node) {
    IRValue value;

    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        case AST_VAR_DECL:
        case AST_ASSIGN:
        case AST_IF:
        case AST_RETURN:
        case AST_WHILE:
        case AST_FOR:
            return irBuilderLowerStatement(builder, node);
        default:
            value = irBuilderLowerRValue(builder, node);
            if (value.has_value) {
                irOperandFree(&value.value);
                irTypeFree(value.type);
            }
            return true;
    }
}

static bool irBuilderLowerBlockNode(IRBuilder *builder, const AstNode *block) {
    size_t i;

    if (block == NULL) {
        return true;
    }

    for (i = 0; i < block->data.block.count; i++) {
        if (!irBuilderLowerStatement(builder, block->data.block.items[i])) {
            return false;
        }
    }

    return true;
}

static bool irBuilderLowerStatement(IRBuilder *builder, const AstNode *node) {
    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        case AST_VAR_DECL:
            return irBuilderLowerVarDecl(builder, node);
        case AST_ASSIGN:
            return irBuilderLowerAssign(builder, node);
        case AST_IF: {
            IRBasicBlock *then_block;
            IRBasicBlock *else_block;
            IRBasicBlock *end_block;
            IRValue condition;
            size_t i;

            then_block = irFunctionCreateBlock(builder->current_function);
            else_block = irFunctionCreateBlock(builder->current_function);
            end_block = irFunctionCreateBlock(builder->current_function);
            if (then_block == NULL || else_block == NULL || end_block == NULL) {
                return false;
            }

            if (!irFunctionAppendBlock(builder->current_function, then_block)
                    || !irFunctionAppendBlock(builder->current_function, else_block)
                    || !irFunctionAppendBlock(builder->current_function, end_block)) {
                return false;
            }

            condition = irBuilderLowerRValue(builder, node->data.if_stmt.condition);
            if (!condition.has_value) {
                return false;
            }
            irTypeFree(condition.type);

            if (!irBuilderEmitCondBranch(builder, condition.value, then_block, else_block)) {
                return false;
            }

            builder->current_block = then_block;
            for (i = 0; i < node->data.if_stmt.then_branch->data.block.count; i++) {
                if (!irBuilderLowerStatement(builder, node->data.if_stmt.then_branch->data.block.items[i])) {
                    return false;
                }
            }
            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, end_block)) {
                return false;
            }

            builder->current_block = else_block;
            if (node->data.if_stmt.else_branch != NULL) {
                for (i = 0; i < node->data.if_stmt.else_branch->data.block.count; i++) {
                    if (!irBuilderLowerStatement(builder, node->data.if_stmt.else_branch->data.block.items[i])) {
                        return false;
                    }
                }
            }
            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, end_block)) {
                return false;
            }

            builder->current_block = end_block;
            return true;
        }
        case AST_WHILE: {
            IRBasicBlock *cond_block = irFunctionCreateBlock(builder->current_function);
            IRBasicBlock *body_block = irFunctionCreateBlock(builder->current_function);
            IRBasicBlock *end_block = irFunctionCreateBlock(builder->current_function);
            IRValue condition;

            if (cond_block == NULL || body_block == NULL || end_block == NULL) {
                return false;
            }

            if (!irFunctionAppendBlock(builder->current_function, cond_block)
                    || !irFunctionAppendBlock(builder->current_function, body_block)
                    || !irFunctionAppendBlock(builder->current_function, end_block)) {
                return false;
            }

            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, cond_block)) {
                return false;
            }

            builder->current_block = cond_block;
            condition = irBuilderLowerRValue(builder, node->data.while_stmt.condition);
            if (!condition.has_value) {
                return false;
            }
            irTypeFree(condition.type);
            if (!irBuilderEmitCondBranch(builder, condition.value, body_block, end_block)) {
                return false;
            }

            if (!irBuilderPushLoopTarget(builder, end_block, cond_block)) {
                return false;
            }
            builder->current_block = body_block;
            if (!irBuilderLowerBlockNode(builder, node->data.while_stmt.body)) {
                irBuilderPopLoopTarget(builder);
                return false;
            }
            irBuilderPopLoopTarget(builder);
            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, cond_block)) {
                return false;
            }

            builder->current_block = end_block;
            return true;
        }
        case AST_FOR: {
            IRScope *saved_scope = builder->current_scope;
            IRScope *loop_scope = irScopeCreate(saved_scope);
            IRBasicBlock *cond_block;
            IRBasicBlock *body_block;
            IRBasicBlock *update_block;
            IRBasicBlock *end_block;

            if (loop_scope == NULL) {
                return false;
            }

            builder->current_scope = loop_scope;
            if (!irBuilderLowerLooseNode(builder, node->data.for_stmt.init)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            cond_block = irFunctionCreateBlock(builder->current_function);
            body_block = irFunctionCreateBlock(builder->current_function);
            update_block = irFunctionCreateBlock(builder->current_function);
            end_block = irFunctionCreateBlock(builder->current_function);
            if (cond_block == NULL || body_block == NULL || update_block == NULL || end_block == NULL) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            if (!irFunctionAppendBlock(builder->current_function, cond_block)
                    || !irFunctionAppendBlock(builder->current_function, body_block)
                    || !irFunctionAppendBlock(builder->current_function, update_block)
                    || !irFunctionAppendBlock(builder->current_function, end_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, cond_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            builder->current_block = cond_block;
            if (node->data.for_stmt.condition != NULL) {
                IRValue condition = irBuilderLowerRValue(builder, node->data.for_stmt.condition);
                if (!condition.has_value) {
                    irScopeFree(loop_scope);
                    builder->current_scope = saved_scope;
                    return false;
                }
                irTypeFree(condition.type);
                if (!irBuilderEmitCondBranch(builder, condition.value, body_block, end_block)) {
                    irScopeFree(loop_scope);
                    builder->current_scope = saved_scope;
                    return false;
                }
            } else if (!irBuilderEmitBranch(builder, body_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            if (!irBuilderPushLoopTarget(builder, end_block, update_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }
            builder->current_block = body_block;
            if (!irBuilderLowerBlockNode(builder, node->data.for_stmt.body)) {
                irBuilderPopLoopTarget(builder);
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }
            irBuilderPopLoopTarget(builder);
            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, update_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            builder->current_block = update_block;
            if (!irBuilderLowerLooseNode(builder, node->data.for_stmt.update)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }
            if (!irBuilderCurrentBlockHasTerminator(builder) && !irBuilderEmitBranch(builder, cond_block)) {
                irScopeFree(loop_scope);
                builder->current_scope = saved_scope;
                return false;
            }

            builder->current_block = end_block;
            irScopeFree(loop_scope);
            builder->current_scope = saved_scope;
            return true;
        }
        case AST_BREAK: {
            IRLoopTarget *loop_target = irBuilderCurrentLoopTarget(builder);
            if (loop_target == NULL) {
                return false;
            }

            return irBuilderEmitBranch(builder, loop_target->break_target);
        }
        case AST_CONTINUE: {
            IRLoopTarget *loop_target = irBuilderCurrentLoopTarget(builder);
            if (loop_target == NULL) {
                return false;
            }

            return irBuilderEmitBranch(builder, loop_target->continue_target);
        }
        case AST_RETURN:
            return irBuilderLowerReturn(builder, node);
        case AST_EXPR_STMT: {
            IRValue value = irBuilderLowerRValue(builder, node->data.expr_stmt.expression);
            if (value.has_value) {
                irOperandFree(&value.value);
                irTypeFree(value.type);
            }
            return true;
        }
        default:
            return true;
    }
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

    if (!irBuilderDeclareBuiltinRuntimeFunctions(builder)) {
        irBuilderFree(builder);
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
            if (!irBuilderPredeclareFunction(builder, node)) {
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
