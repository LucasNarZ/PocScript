#include "ir.h"

#include <stdlib.h>

static void irInstructionFreePayload(IRInstruction *instruction) {
    size_t i;

    switch (instruction->kind) {
        case IR_INSTR_CONST:
            irOperandFree(&instruction->data.const_instr.value);
            break;
        case IR_INSTR_GLOBAL_ADDR:
            break;
        case IR_INSTR_ALLOCA:
            irTypeFree(instruction->data.alloca.allocated_type);
            break;
        case IR_INSTR_LOAD:
            irOperandFree(&instruction->data.load.address);
            break;
        case IR_INSTR_STORE:
            irOperandFree(&instruction->data.store.address);
            irOperandFree(&instruction->data.store.value);
            break;
        case IR_INSTR_GEP:
            irOperandFree(&instruction->data.gep.base);
            for (i = 0; i < instruction->data.gep.index_count; i++) {
                irOperandFree(&instruction->data.gep.indices[i]);
            }
            free(instruction->data.gep.indices);
            break;
        case IR_INSTR_ADD:
        case IR_INSTR_SUB:
        case IR_INSTR_MUL:
        case IR_INSTR_DIV:
        case IR_INSTR_AND:
        case IR_INSTR_OR:
        case IR_INSTR_CMP_EQ:
        case IR_INSTR_CMP_NE:
        case IR_INSTR_CMP_LT:
        case IR_INSTR_CMP_LE:
        case IR_INSTR_CMP_GT:
        case IR_INSTR_CMP_GE:
            irOperandFree(&instruction->data.binary.left);
            irOperandFree(&instruction->data.binary.right);
            break;
        case IR_INSTR_NEG:
        case IR_INSTR_NOT:
            irOperandFree(&instruction->data.unary.operand);
            break;
        case IR_INSTR_BR:
            break;
        case IR_INSTR_COND_BR:
            irOperandFree(&instruction->data.cond_br.condition);
            break;
        case IR_INSTR_RET:
            if (instruction->data.ret.has_value) {
                irOperandFree(&instruction->data.ret.value);
            }
            break;
        case IR_INSTR_CALL:
            free(instruction->data.call.callee);
            for (i = 0; i < instruction->data.call.arg_count; i++) {
                irOperandFree(&instruction->data.call.args[i]);
            }
            free(instruction->data.call.args);
            break;
        case IR_INSTR_CAST:
            irOperandFree(&instruction->data.cast.value);
            irTypeFree(instruction->data.cast.target_type);
            break;
    }
}

IRInstruction *irInstructionCreate(IRInstructionKind kind) {
    IRInstruction *instruction = calloc(1, sizeof(IRInstruction));

    if (instruction == NULL) {
        return NULL;
    }

    instruction->kind = kind;
    return instruction;
}

void irInstructionFree(IRInstruction *instruction) {
    if (instruction == NULL) {
        return;
    }

    irInstructionFreePayload(instruction);
    irTypeFree(instruction->result_type);
    free(instruction);
}
