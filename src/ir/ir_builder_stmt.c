#include "ir_builder_internal.h"

#include <stdlib.h>

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
    } else if (node->data.var_decl.initializer != NULL) {
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

static bool irBuilderLowerIfStatement(IRBuilder *builder, const AstNode *node) {
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

static bool irBuilderLowerWhileStatement(IRBuilder *builder, const AstNode *node) {
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

static bool irBuilderLowerForStatement(IRBuilder *builder, const AstNode *node) {
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

static bool irBuilderLowerBreakStatement(IRBuilder *builder) {
    IRLoopTarget *loop_target = irBuilderCurrentLoopTarget(builder);

    if (loop_target == NULL) {
        return false;
    }

    return irBuilderEmitBranch(builder, loop_target->break_target);
}

static bool irBuilderLowerContinueStatement(IRBuilder *builder) {
    IRLoopTarget *loop_target = irBuilderCurrentLoopTarget(builder);

    if (loop_target == NULL) {
        return false;
    }

    return irBuilderEmitBranch(builder, loop_target->continue_target);
}

static bool irBuilderLowerExprStatement(IRBuilder *builder, const AstNode *node) {
    IRValue value = irBuilderLowerRValue(builder, node->data.expr_stmt.expression);

    if (value.has_value) {
        irOperandFree(&value.value);
        irTypeFree(value.type);
    }

    return true;
}

bool irBuilderLowerStatement(IRBuilder *builder, const AstNode *node) {
    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        case AST_VAR_DECL:
            return irBuilderLowerVarDecl(builder, node);
        case AST_ASSIGN:
            return irBuilderLowerAssign(builder, node);
        case AST_IF:
            return irBuilderLowerIfStatement(builder, node);
        case AST_WHILE:
            return irBuilderLowerWhileStatement(builder, node);
        case AST_FOR:
            return irBuilderLowerForStatement(builder, node);
        case AST_BREAK:
            return irBuilderLowerBreakStatement(builder);
        case AST_CONTINUE:
            return irBuilderLowerContinueStatement(builder);
        case AST_RETURN:
            return irBuilderLowerReturn(builder, node);
        case AST_EXPR_STMT:
            return irBuilderLowerExprStatement(builder, node);
        default:
            return true;
    }
}
