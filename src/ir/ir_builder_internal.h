#ifndef IR_BUILDER_INTERNAL_H
#define IR_BUILDER_INTERNAL_H

#include "ir.h"

char *irBuilderCopyString(const char *value);
char *irBuilderUnquoteString(const char *value);
IRType *irBuilderTypeFromAst(const AstNode *type_node);
IROperand irBuilderCloneOperand(const IROperand *operand);
void irBuilderFreeOperandArray(IROperand *operands, size_t count);
size_t irBuilderArrayDepth(const IRType *type);
IRType *irBuilderInferArrayTypeFromLiteral(const IRType *declared_type, const AstNode *literal);
bool irBuilderPushLoopTarget(IRBuilder *builder, IRBasicBlock *break_target, IRBasicBlock *continue_target);
void irBuilderPopLoopTarget(IRBuilder *builder);
IRLoopTarget *irBuilderCurrentLoopTarget(IRBuilder *builder);
unsigned int irBuilderReserveResult(IRBuilder *builder);
bool irBuilderAppendInstruction(IRBuilder *builder, IRInstruction *instruction);
size_t irBuilderStringLiteralLength(const char *value);
unsigned int irBuilderAddStringStorage(IRBuilder *builder, const char *value);
bool irBuilderMaterializeParameter(IRBuilder *builder, IRFunction *function, const IRParameter *parameter, size_t index);
IRValue irBuilderLiteralValue(IRTypeKind kind, IRLiteral *literal);
IRValue irBuilderEmitLoad(IRBuilder *builder, IROperand address, IRType *value_type);
IRValue irBuilderEmitBinary(IRBuilder *builder, IRInstructionKind kind, IROperand left, IROperand right, IRType *type);
IRValue irBuilderEmitUnary(IRBuilder *builder, IRInstructionKind kind, IROperand operand, IRType *type);
IRValue irBuilderEmitGep(IRBuilder *builder, IROperand base, IROperand *indices, size_t index_count, IRType *result_type);
bool irBuilderEmitStore(IRBuilder *builder, IROperand address, IROperand value);
bool irBuilderEmitBranch(IRBuilder *builder, IRBasicBlock *target);
bool irBuilderEmitCondBranch(IRBuilder *builder, IROperand condition, IRBasicBlock *then_block, IRBasicBlock *else_block);
bool irBuilderCurrentBlockHasTerminator(IRBuilder *builder);
IRValue irBuilderLowerLValue(IRBuilder *builder, const AstNode *node);
IRValue irBuilderLowerRValue(IRBuilder *builder, const AstNode *node);
bool irBuilderLowerStatement(IRBuilder *builder, const AstNode *node);

#endif
