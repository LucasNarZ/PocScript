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

IRType *irTypeCreate(IRTypeKind kind) {
    IRType *type = calloc(1, sizeof(IRType));

    if (type == NULL) {
        return NULL;
    }

    type->kind = kind;
    return type;
}

IRType *irTypeCreateArray(IRType *element_type) {
    IRType *type = irTypeCreate(IR_TYPE_ARRAY);

    if (type == NULL) {
        irTypeFree(element_type);
        return NULL;
    }

    type->element_type = element_type;
    return type;
}

IRType *irTypeCreateSizedArray(IRType *element_type, size_t array_size) {
    IRType *type = irTypeCreateArray(element_type);

    if (type == NULL) {
        return NULL;
    }

    type->array_size = array_size;
    type->has_array_size = true;
    return type;
}

IRType *irTypeCreatePointer(IRType *element_type) {
    IRType *type = irTypeCreate(IR_TYPE_POINTER);

    if (type == NULL) {
        irTypeFree(element_type);
        return NULL;
    }

    type->element_type = element_type;
    return type;
}

IRType *irTypeClone(const IRType *type) {
    IRType *copy;

    if (type == NULL) {
        return NULL;
    }

    copy = irTypeCreate(type->kind);
    if (copy == NULL) {
        return NULL;
    }

    if (type->element_type != NULL) {
        copy->element_type = irTypeClone(type->element_type);
        if (copy->element_type == NULL) {
            irTypeFree(copy);
            return NULL;
        }
    }

    copy->array_size = type->array_size;
    copy->has_array_size = type->has_array_size;

    return copy;
}

void irTypeFree(IRType *type) {
    if (type == NULL) {
        return;
    }

    irTypeFree(type->element_type);
    free(type);
}

IRLiteral *irLiteralCreateInt(long value) {
    IRLiteral *literal = calloc(1, sizeof(IRLiteral));

    if (literal == NULL) {
        return NULL;
    }

    literal->kind = IR_LITERAL_INT;
    literal->data.int_value = value;
    return literal;
}

IRLiteral *irLiteralCreateFloat(double value) {
    IRLiteral *literal = calloc(1, sizeof(IRLiteral));

    if (literal == NULL) {
        return NULL;
    }

    literal->kind = IR_LITERAL_FLOAT;
    literal->data.float_value = value;
    return literal;
}

IRLiteral *irLiteralCreateString(const char *value) {
    IRLiteral *literal = calloc(1, sizeof(IRLiteral));

    if (literal == NULL) {
        return NULL;
    }

    literal->kind = IR_LITERAL_STRING;
    literal->data.string_value = irCopyString(value);
    if (value != NULL && literal->data.string_value == NULL) {
        free(literal);
        return NULL;
    }

    return literal;
}

IRLiteral *irLiteralCreateBool(bool value) {
    IRLiteral *literal = calloc(1, sizeof(IRLiteral));

    if (literal == NULL) {
        return NULL;
    }

    literal->kind = IR_LITERAL_BOOL;
    literal->data.bool_value = value;
    return literal;
}

void irLiteralFree(IRLiteral *literal) {
    if (literal == NULL) {
        return;
    }

    if (literal->kind == IR_LITERAL_STRING) {
        free(literal->data.string_value);
    }

    free(literal);
}

IROperand irOperandCreateLocal(IRType *type, unsigned int local_id) {
    IROperand operand;

    operand.type = type;
    operand.kind = IR_OPERAND_LOCAL;
    operand.data.local_id = local_id;
    return operand;
}

IROperand irOperandCreateParam(IRType *type, unsigned int param_id) {
    IROperand operand;

    operand.type = type;
    operand.kind = IR_OPERAND_PARAM;
    operand.data.param_id = param_id;
    return operand;
}

IROperand irOperandCreateGlobal(IRType *type, unsigned int global_id) {
    IROperand operand;

    operand.type = type;
    operand.kind = IR_OPERAND_GLOBAL;
    operand.data.global_id = global_id;
    return operand;
}

IROperand irOperandCreateLiteral(IRType *type, IRLiteral *literal) {
    IROperand operand;

    operand.type = type;
    operand.kind = IR_OPERAND_LITERAL;
    operand.data.literal = literal;
    return operand;
}

IROperand irOperandCreateBlock(unsigned int block_id) {
    IROperand operand;

    operand.type = NULL;
    operand.kind = IR_OPERAND_BLOCK;
    operand.data.block_id = block_id;
    return operand;
}

IRValue irValueEmpty(void) {
    IRValue value;

    memset(&value, 0, sizeof(IRValue));
    return value;
}

void irOperandFree(IROperand *operand) {
    if (operand == NULL) {
        return;
    }

    irTypeFree(operand->type);
    if (operand->kind == IR_OPERAND_LITERAL) {
        irLiteralFree(operand->data.literal);
    }

    operand->type = NULL;
}
