#include "ir_builder_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *irBuilderCopyString(const char *value) {
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

char *irBuilderUnquoteString(const char *value) {
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

IRType *irBuilderTypeFromAst(const AstNode *type_node) {
    IRType *element_type;
    const AstNode *element_type_node;

    if (type_node == NULL) {
        return NULL;
    }

    if (type_node->type == AST_TYPE_POINTER) {
        element_type = irBuilderTypeFromAst(type_node->data.type_pointer.target_type);
        if (element_type == NULL) {
            return NULL;
        }

        return irTypeCreatePointer(element_type);
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

IROperand irBuilderCloneOperand(const IROperand *operand) {
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

void irBuilderFreeOperandArray(IROperand *operands, size_t count) {
    size_t i;

    if (operands == NULL) {
        return;
    }

    for (i = 0; i < count; i++) {
        irOperandFree(&operands[i]);
    }

    free(operands);
}

size_t irBuilderArrayDepth(const IRType *type) {
    size_t depth = 0;
    const IRType *current = type;

    while (current != NULL && current->kind == IR_TYPE_ARRAY) {
        depth++;
        current = current->element_type;
    }

    return depth;
}

IRType *irBuilderInferArrayTypeFromLiteral(const IRType *declared_type, const AstNode *literal) {
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

bool irBuilderPushLoopTarget(IRBuilder *builder, IRBasicBlock *break_target, IRBasicBlock *continue_target) {
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

void irBuilderPopLoopTarget(IRBuilder *builder) {
    if (builder->loop_target_count > 0) {
        builder->loop_target_count--;
    }
}

IRLoopTarget *irBuilderCurrentLoopTarget(IRBuilder *builder) {
    if (builder->loop_target_count == 0) {
        return NULL;
    }

    return &builder->loop_targets[builder->loop_target_count - 1];
}

bool irBuilderAppendInstruction(IRBuilder *builder, IRInstruction *instruction) {
    if (instruction == NULL) {
        return false;
    }

    if (!irBasicBlockAppendInstruction(builder->current_block, instruction)) {
        irInstructionFree(instruction);
        return false;
    }

    return true;
}

bool irBuilderCurrentBlockHasTerminator(IRBuilder *builder) {
    IRInstruction *instruction;

    if (builder == NULL || builder->current_block == NULL || builder->current_block->instruction_count == 0) {
        return false;
    }

    instruction = builder->current_block->instructions[builder->current_block->instruction_count - 1];
    return instruction->kind == IR_INSTR_RET || instruction->kind == IR_INSTR_BR || instruction->kind == IR_INSTR_COND_BR;
}

unsigned int irBuilderReserveResult(IRBuilder *builder) {
    return irFunctionReserveLocalId(builder->current_function);
}

size_t irBuilderStringLiteralLength(const char *value) {
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

IRValue irBuilderLiteralValue(IRTypeKind kind, IRLiteral *literal) {
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

IRValue irBuilderEmitLoad(IRBuilder *builder, IROperand address, IRType *value_type) {
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

IRValue irBuilderEmitBinary(IRBuilder *builder, IRInstructionKind kind, IROperand left, IROperand right, IRType *type) {
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

IRValue irBuilderEmitUnary(IRBuilder *builder, IRInstructionKind kind, IROperand operand, IRType *type) {
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

IRValue irBuilderEmitGep(IRBuilder *builder, IROperand base, IROperand *indices, size_t index_count, IRType *result_type) {
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

bool irBuilderEmitStore(IRBuilder *builder, IROperand address, IROperand value) {
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

bool irBuilderEmitBranch(IRBuilder *builder, IRBasicBlock *target) {
    IRInstruction *instruction = irInstructionCreate(IR_INSTR_BR);

    if (instruction == NULL) {
        return false;
    }

    instruction->data.br.target_block_id = target->id;
    return irBuilderAppendInstruction(builder, instruction);
}

bool irBuilderEmitCondBranch(IRBuilder *builder, IROperand condition, IRBasicBlock *then_block, IRBasicBlock *else_block) {
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
