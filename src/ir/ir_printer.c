#define _GNU_SOURCE

#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IRGlobal *irPrinterFindGlobalById(const IRModule *module, unsigned int global_id) {
    size_t i;

    for (i = 0; i < module->global_count; i++) {
        if (module->global_items[i]->id == global_id) {
            return module->global_items[i];
        }
    }

    return NULL;
}

static void irPrintEscapedString(FILE *out, const char *value) {
    const unsigned char *cursor = (const unsigned char *) value;

    while (*cursor != '\0') {
        if (*cursor == '\\') {
            fputs("\\5C", out);
        } else if (*cursor == '"') {
            fputs("\\22", out);
        } else if (*cursor == '\n') {
            fputs("\\0A", out);
        } else if (*cursor == '\t') {
            fputs("\\09", out);
        } else if (*cursor < 32 || *cursor > 126) {
            fprintf(out, "\\%02X", *cursor);
        } else {
            fputc(*cursor, out);
        }
        cursor++;
    }
}

static void irPrintType(FILE *out, const IRType *type) {
    if (type == NULL) {
        fputs("void", out);
        return;
    }

    switch (type->kind) {
        case IR_TYPE_VOID:
            fputs("void", out);
            break;
        case IR_TYPE_INT:
            fputs("i32", out);
            break;
        case IR_TYPE_FLOAT:
            fputs("double", out);
            break;
        case IR_TYPE_CHAR:
            fputs("i8", out);
            break;
        case IR_TYPE_BOOL:
            fputs("i1", out);
            break;
        case IR_TYPE_STRING:
            fputs("i8*", out);
            break;
        case IR_TYPE_ARRAY:
            fprintf(out, "[%zu x ", type->has_array_size ? type->array_size : 0UL);
            irPrintType(out, type->element_type);
            fputc(']', out);
            break;
        case IR_TYPE_POINTER:
            irPrintType(out, type->element_type);
            fputc('*', out);
            break;
    }
}

static void irPrintZeroValue(FILE *out, const IRType *type) {
    switch (type->kind) {
        case IR_TYPE_FLOAT:
            fputs("0.0", out);
            break;
        case IR_TYPE_BOOL:
        case IR_TYPE_CHAR:
        case IR_TYPE_INT:
            fputs("0", out);
            break;
        case IR_TYPE_STRING:
        case IR_TYPE_POINTER:
            fputs("null", out);
            break;
        case IR_TYPE_ARRAY:
            fputs("zeroinitializer", out);
            break;
        case IR_TYPE_VOID:
            break;
    }
}

static void irPrintOperand(FILE *out, const IRModule *module, const IROperand *operand) {
    if (operand == NULL) {
        return;
    }

    switch (operand->kind) {
        case IR_OPERAND_LOCAL:
            fprintf(out, "%%t%u", operand->data.local_id);
            break;
        case IR_OPERAND_PARAM:
            fprintf(out, "%%p%u", operand->data.param_id);
            break;
        case IR_OPERAND_GLOBAL:
        {
            IRGlobal *global = module != NULL ? irPrinterFindGlobalById(module, operand->data.global_id) : NULL;
            fprintf(out, "@%s", global != NULL ? global->name : "g");
            break;
        }
        case IR_OPERAND_LITERAL:
            switch (operand->data.literal->kind) {
                case IR_LITERAL_INT:
                    fprintf(out, "%ld", operand->data.literal->data.int_value);
                    break;
                case IR_LITERAL_FLOAT:
                    fprintf(out, "%f", operand->data.literal->data.float_value);
                    break;
                case IR_LITERAL_STRING:
                    fputs("null", out);
                    break;
                case IR_LITERAL_BOOL:
                    fprintf(out, "%d", operand->data.literal->data.bool_value ? 1 : 0);
                    break;
            }
            break;
        case IR_OPERAND_BLOCK:
            fprintf(out, "%%bb%u", operand->data.block_id);
            break;
    }
}

static void irPrintGlobal(FILE *out, const IRModule *module, const IRGlobal *global) {
    if (global->is_constant) {
        fprintf(out, "@%s = private unnamed_addr constant ", global->name);
        irPrintType(out, global->type);
        fputs(" c\"", out);
        irPrintEscapedString(out, global->initializer != NULL ? global->initializer->data.string_value : "");
        fputs("\\00\"\n", out);
        return;
    }

    fprintf(out, "@%s = global ", global->name);
    irPrintType(out, global->type);
    fputc(' ', out);

    if (global->type != NULL && global->type->kind == IR_TYPE_STRING && global->initializer != NULL && global->initializer->kind == IR_LITERAL_STRING && global->has_storage_global) {
        IRGlobal *storage = irPrinterFindGlobalById(module, global->storage_global_id);

        fputs("getelementptr inbounds (", out);
        irPrintType(out, storage->type);
        fputs(", ", out);
        irPrintType(out, storage->type);
        fprintf(out, "* @%s, i32 0, i32 0)\n", storage->name);
        return;
    }

    if (global->initializer == NULL) {
        irPrintZeroValue(out, global->type);
        fputc('\n', out);
        return;
    }

    switch (global->initializer->kind) {
        case IR_LITERAL_INT:
            fprintf(out, "%ld\n", global->initializer->data.int_value);
            break;
        case IR_LITERAL_FLOAT:
            fprintf(out, "%f\n", global->initializer->data.float_value);
            break;
        case IR_LITERAL_BOOL:
            fprintf(out, "%d\n", global->initializer->data.bool_value ? 1 : 0);
            break;
        case IR_LITERAL_STRING:
            fputs("null\n", out);
            break;
    }
}

static void irPrintInstruction(FILE *out, const IRModule *module, const IRInstruction *instruction) {
    switch (instruction->kind) {
        case IR_INSTR_ALLOCA:
            fprintf(out, "  %%t%u = alloca ", instruction->result_id);
            irPrintType(out, instruction->data.alloca.allocated_type);
            fputc('\n', out);
            break;
        case IR_INSTR_LOAD:
            fprintf(out, "  %%t%u = load ", instruction->result_id);
            irPrintType(out, instruction->result_type);
            fputs(", ", out);
            irPrintType(out, instruction->data.load.address.type);
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.load.address);
            fputc('\n', out);
            break;
        case IR_INSTR_STORE:
            fputs("  store ", out);
            irPrintType(out, instruction->data.store.value.type);
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.store.value);
            fputs(", ", out);
            irPrintType(out, instruction->data.store.address.type);
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.store.address);
            fputc('\n', out);
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
            if (instruction->kind == IR_INSTR_CMP_EQ
                    || instruction->kind == IR_INSTR_CMP_NE
                    || instruction->kind == IR_INSTR_CMP_GT
                    || instruction->kind == IR_INSTR_CMP_LT
                    || instruction->kind == IR_INSTR_CMP_LE
                    || instruction->kind == IR_INSTR_CMP_GE) {
                fprintf(out, "  %%t%u = ", instruction->result_id);
                fputs(
                    instruction->kind == IR_INSTR_CMP_EQ ? "icmp eq " :
                    instruction->kind == IR_INSTR_CMP_NE ? "icmp ne " :
                    instruction->kind == IR_INSTR_CMP_GT ? "icmp sgt " :
                    instruction->kind == IR_INSTR_CMP_LT ? "icmp slt " :
                    instruction->kind == IR_INSTR_CMP_LE ? "icmp sle " :
                    "icmp sge ",
                    out
                );
                irPrintType(out, instruction->data.binary.left.type);
            } else {
                fprintf(out, "  %%t%u = %s ", instruction->result_id,
                    instruction->kind == IR_INSTR_ADD ? "add" :
                    instruction->kind == IR_INSTR_SUB ? "sub" :
                    instruction->kind == IR_INSTR_MUL ? "mul" :
                    instruction->kind == IR_INSTR_DIV ? "sdiv" :
                    instruction->kind == IR_INSTR_AND ? "and" : "or");
                irPrintType(out, instruction->result_type);
            }
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.binary.left);
            fputs(", ", out);
            irPrintOperand(out, module, &instruction->data.binary.right);
            fputc('\n', out);
            break;
        case IR_INSTR_NEG:
            fprintf(out, "  %%t%u = sub ", instruction->result_id);
            irPrintType(out, instruction->result_type);
            fputs(" 0, ", out);
            irPrintOperand(out, module, &instruction->data.unary.operand);
            fputc('\n', out);
            break;
        case IR_INSTR_NOT:
            fprintf(out, "  %%t%u = xor ", instruction->result_id);
            irPrintType(out, instruction->result_type);
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.unary.operand);
            fputs(", 1\n", out);
            break;
        case IR_INSTR_GEP:
            fprintf(out, "  %%t%u = getelementptr inbounds ", instruction->result_id);
            irPrintType(out, instruction->data.gep.base.type->element_type);
            fputs(", ", out);
            irPrintType(out, instruction->data.gep.base.type);
            fputc(' ', out);
            irPrintOperand(out, module, &instruction->data.gep.base);
            for (size_t i = 0; i < instruction->data.gep.index_count; i++) {
                fputs(", ", out);
                irPrintType(out, instruction->data.gep.indices[i].type);
                fputc(' ', out);
                irPrintOperand(out, module, &instruction->data.gep.indices[i]);
            }
            fputc('\n', out);
            break;
        case IR_INSTR_BR:
            fprintf(out, "  br label %%bb%u\n", instruction->data.br.target_block_id);
            break;
        case IR_INSTR_COND_BR:
            fputs("  br i1 ", out);
            irPrintOperand(out, module, &instruction->data.cond_br.condition);
            fprintf(out, ", label %%bb%u, label %%bb%u\n", instruction->data.cond_br.then_block_id, instruction->data.cond_br.else_block_id);
            break;
        case IR_INSTR_CALL:
            fputs("  ", out);
            if (instruction->has_result) {
                fprintf(out, "%%t%u = ", instruction->result_id);
            }
            fputs("call ", out);
            if (instruction->has_result) {
                irPrintType(out, instruction->result_type);
            } else {
                fputs("void", out);
            }
            fprintf(out, " @%s(", instruction->data.call.callee);
            for (size_t i = 0; i < instruction->data.call.arg_count; i++) {
                if (i > 0) {
                    fputs(", ", out);
                }
                irPrintType(out, instruction->data.call.args[i].type);
                fputc(' ', out);
                irPrintOperand(out, module, &instruction->data.call.args[i]);
            }
            fputs(")\n", out);
            break;
        case IR_INSTR_RET:
            if (!instruction->data.ret.has_value) {
                fputs("  ret void\n", out);
            } else {
                fputs("  ret ", out);
                irPrintType(out, instruction->data.ret.value.type);
                fputc(' ', out);
                irPrintOperand(out, module, &instruction->data.ret.value);
                fputc('\n', out);
            }
            break;
        default:
            break;
    }
}

static void irPrintFunction(FILE *out, const IRModule *module, const IRFunction *function) {
    size_t i;
    size_t j;

    fputs("define ", out);
    irPrintType(out, function->return_type);
    fprintf(out, " @%s(", function->name);
    for (i = 0; i < function->param_count; i++) {
        if (i > 0) {
            fputs(", ", out);
        }
        irPrintType(out, function->params[i].type);
        fprintf(out, " %%p%zu", i + 1);
    }
    fputs(") {\n", out);

    if (function->block_count == 0) {
        fputs("bb0:\n", out);
        if (function->return_type != NULL && function->return_type->kind == IR_TYPE_VOID) {
            fputs("  ret void\n", out);
        } else {
            fputs("  ret ", out);
            irPrintType(out, function->return_type);
            fputc(' ', out);
            irPrintZeroValue(out, function->return_type);
            fputc('\n', out);
        }
        fputs("}\n", out);
        return;
    }

    for (i = 0; i < function->block_count; i++) {
        fprintf(out, "bb%u:\n", function->blocks[i]->id);
        for (j = 0; j < function->blocks[i]->instruction_count; j++) {
            irPrintInstruction(out, module, function->blocks[i]->instructions[j]);
        }
    }
    fputs("}\n", out);
}

static bool irPrinterHasFunctionDefinition(const IRModule *module, const char *name) {
    size_t i;

    for (i = 0; i < module->function_count; i++) {
        if (strcmp(module->functions[i]->name, name) == 0) {
            return true;
        }
    }

    return false;
}

static bool irPrintExternalDeclarations(FILE *out, const IRModule *module) {
    size_t i;
    bool printed = false;

    if (module == NULL || module->global_scope == NULL || module->global_scope->symbols == NULL) {
        return false;
    }

    for (i = 0; i < module->global_scope->symbols->bucket_count; i++) {
        IRSymbol *symbol = module->global_scope->symbols->buckets[i];
        while (symbol != NULL) {
            if (symbol->kind == IR_SYMBOL_FUNCTION && !irPrinterHasFunctionDefinition(module, symbol->name)) {
                size_t j;

                fputs("declare ", out);
                irPrintType(out, symbol->type);
                fprintf(out, " @%s(", symbol->name);
                for (j = 0; j < symbol->data.function.param_count; j++) {
                    if (j > 0) {
                        fputs(", ", out);
                    }
                    irPrintType(out, symbol->data.function.param_types[j]);
                }
                fputs(")\n", out);
                printed = true;
            }
            symbol = symbol->next;
        }
    }

    return printed;
}

char *irPrintModuleToString(const IRModule *module) {
    char *buffer = NULL;
    size_t size = 0;
    FILE *stream;
    size_t i;
    bool has_external_declarations;

    if (module == NULL) {
        return NULL;
    }

    stream = open_memstream(&buffer, &size);
    if (stream == NULL) {
        return NULL;
    }

    for (i = 0; i < module->global_count; i++) {
        irPrintGlobal(stream, module, module->global_items[i]);
    }
    has_external_declarations = irPrintExternalDeclarations(stream, module);
    if ((module->global_count > 0 || has_external_declarations) && module->function_count > 0) {
        fputc('\n', stream);
    }
    for (i = 0; i < module->function_count; i++) {
        irPrintFunction(stream, module, module->functions[i]);
        if (i + 1 < module->function_count) {
            fputc('\n', stream);
        }
    }

    fclose(stream);
    return buffer;
}

bool irPrintModuleToFile(const IRModule *module, const char *path) {
    char *text;
    FILE *file;
    size_t length;

    if (module == NULL || path == NULL) {
        return false;
    }

    text = irPrintModuleToString(module);
    if (text == NULL) {
        return false;
    }

    file = fopen(path, "w");
    if (file == NULL) {
        free(text);
        return false;
    }

    length = strlen(text);
    if (fwrite(text, 1, length, file) != length) {
        fclose(file);
        free(text);
        return false;
    }

    fclose(file);
    free(text);
    return true;
}
