#include "ir_builder_internal.h"

#include <stdlib.h>

static IRSymbol *irBuilderLookupLocal(IRBuilder *builder, const char *name) {
    return irScopeLookup(builder->current_scope, name);
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

IRValue irBuilderLowerLValue(IRBuilder *builder, const AstNode *node) {
    IRValue pointer_value;

    if (node == NULL) {
        return irValueEmpty();
    }

    if (node->type == AST_IDENTIFIER) {
        return irBuilderLowerIdentifier(builder, node);
    }

    if (node->type == AST_ARRAY_ACCESS) {
        return irBuilderLowerArrayAccess(builder, node);
    }

    if (node->type == AST_UNARY && node->data.unary.op == AST_UNARY_DEREF) {
        pointer_value = irBuilderLowerRValue(builder, node->data.unary.operand);
        if (!pointer_value.has_value || pointer_value.type == NULL || pointer_value.type->kind != IR_TYPE_POINTER || pointer_value.type->element_type == NULL) {
            irTypeFree(pointer_value.type);
            if (pointer_value.has_value) {
                irOperandFree(&pointer_value.value);
            }
            return irValueEmpty();
        }

        {
            IRValue result = irValueEmpty();
            result.type = irTypeClone(pointer_value.type->element_type);
            result.address = pointer_value.value;
            result.has_address = true;
            irTypeFree(pointer_value.type);
            return result;
        }
    }

    return irValueEmpty();
}

static IRValue irBuilderLowerLiteralRValue(const AstNode *node) {
    if (node == NULL) {
        return irValueEmpty();
    }

    switch (node->type) {
        case AST_INT_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_INT, irLiteralCreateInt(node->data.int_literal.value));
        case AST_FLOAT_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_FLOAT, irLiteralCreateFloat(node->data.float_literal.value));
        case AST_BOOL_LITERAL:
            return irBuilderLiteralValue(IR_TYPE_BOOL, irLiteralCreateBool(node->data.bool_literal.value));
        default:
            return irValueEmpty();
    }
}

static IRValue irBuilderLowerStringLiteralRValue(IRBuilder *builder, const AstNode *node) {
    IRValue result = irValueEmpty();
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

static IRValue irBuilderLowerLoadedRValue(IRBuilder *builder, const AstNode *node, IRValue (*lower_address)(IRBuilder *, const AstNode *)) {
    IRValue result = lower_address(builder, node);

    if (!result.has_address || result.type == NULL) {
        return irValueEmpty();
    }

    return irBuilderEmitLoad(builder, result.address, result.type);
}

static IRValue irBuilderLowerUnaryRValue(IRBuilder *builder, const AstNode *node) {
    if (node->data.unary.op == AST_UNARY_ADDRESS_OF) {
        IRValue addressed = irBuilderLowerLValue(builder, node->data.unary.operand);
        IRValue result = irValueEmpty();

        if (!addressed.has_address || addressed.address.type == NULL) {
            irTypeFree(addressed.type);
            if (addressed.has_address) {
                irOperandFree(&addressed.address);
            }
            return irValueEmpty();
        }

        result.type = irTypeClone(addressed.address.type);
        result.value = addressed.address;
        result.has_value = true;
        irTypeFree(addressed.type);
        return result;
    }

    if (node->data.unary.op == AST_UNARY_DEREF) {
        IRValue dereferenced = irBuilderLowerLValue(builder, node);

        if (!dereferenced.has_address || dereferenced.type == NULL) {
            return irValueEmpty();
        }

        return irBuilderEmitLoad(builder, dereferenced.address, dereferenced.type);
    }

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

static IRValue irBuilderLowerCallRValue(IRBuilder *builder, const AstNode *node) {
    IRValue result = irValueEmpty();
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

static IRValue irBuilderLowerBinaryRValue(IRBuilder *builder, const AstNode *node) {
    IRValue left = irBuilderLowerRValue(builder, node->data.binary.left);
    IRValue right = irBuilderLowerRValue(builder, node->data.binary.right);
    IRValue binary_result;

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
}

IRValue irBuilderLowerRValue(IRBuilder *builder, const AstNode *node) {
    if (node == NULL) {
        return irValueEmpty();
    }

    switch (node->type) {
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_BOOL_LITERAL:
            return irBuilderLowerLiteralRValue(node);
        case AST_STRING_LITERAL:
            return irBuilderLowerStringLiteralRValue(builder, node);
        case AST_IDENTIFIER:
            return irBuilderLowerLoadedRValue(builder, node, irBuilderLowerIdentifier);
        case AST_ARRAY_ACCESS:
            return irBuilderLowerLoadedRValue(builder, node, irBuilderLowerArrayAccess);
        case AST_UNARY:
            return irBuilderLowerUnaryRValue(builder, node);
        case AST_CALL:
            return irBuilderLowerCallRValue(builder, node);
        case AST_BINARY:
            return irBuilderLowerBinaryRValue(builder, node);
        default:
            return irValueEmpty();
    }
}
