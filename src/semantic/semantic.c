#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void collectSymbols(AstNode *node, Scope *scope, SemanticResult *result);

typedef struct {
    const char *function_name;
    SemanticType *return_type;
} FunctionContext;

typedef enum {
    FLOW_CONTINUES,
    FLOW_RETURNS
} FlowState;

typedef struct {
    SemanticResult *result;
    FunctionContext *current_function;
    int loop_depth;
} SemanticContext;

static FlowState validateNode(AstNode *node, Scope *scope, SemanticContext *ctx);
static SemanticType *checkExpression(AstNode *node, Scope *scope, SemanticContext *ctx);
static SemanticType *checkArrayAccess(AstNode *node, Scope *scope, SemanticContext *ctx);

static bool isAddressableExpression(AstNode *node) {
    if (node == NULL) {
        return false;
    }

    return node->type == AST_IDENTIFIER || node->type == AST_ARRAY_ACCESS;
}

static void appendCategorizedError(AstNode *node, SemanticResult *result, SemanticErrorKind kind, const char *message) {
    int line = 0;
    int column = 0;

    if (node != NULL) {
        line = node->line;
        column = node->column;
    }

    semanticErrorListAppend(&result->errors, kind, line, column, message);
}

static void appendDuplicateDeclarationError(AstNode *node, SemanticResult *result, const char *name) {
    char message[256];

    snprintf(message, sizeof(message), "duplicate declaration of '%s'", name);
    appendCategorizedError(node, result, SEMANTIC_ERROR_DECLARATION, message);
}

static void collectBlock(AstNode *node, Scope *parentScope, SemanticResult *result) {
    Scope *blockScope = scopeCreate(parentScope);
    size_t i;

    if (blockScope == NULL) {
        return;
    }

    for (i = 0; i < node->data.block.count; i++) {
        collectSymbols(node->data.block.items[i], blockScope, result); }

    scopeFree(blockScope);
}

static void collectFunction(AstNode *node, Scope *scope, SemanticResult *result) {
    SemanticType **params = NULL;
    SemanticType *returnType;
    Scope *functionScope;
    Symbol *symbol;
    size_t i;

    if (node->data.func_decl.param_count > 0) {
        params = calloc(node->data.func_decl.param_count, sizeof(SemanticType *));
        if (params == NULL) {
            return;
        }
    }

    for (i = 0; i < node->data.func_decl.param_count; i++) {
        params[i] = semanticTypeFromAst(node->data.func_decl.params[i]->data.param.declared_type);
    }

    returnType = semanticTypeFromAst(node->data.func_decl.return_type);

    symbol = symbolCreateFunction(node->data.func_decl.name, returnType, params, node->data.func_decl.param_count);
    for (i = 0; i < node->data.func_decl.param_count; i++) {
        semanticTypeFree(params[i]);
    }
    free(params);

    if (symbol == NULL) {
        return;
    }

    if (!scopeDeclare(scope, symbol)) {
        appendDuplicateDeclarationError(node, result, node->data.func_decl.name);
        symbolFree(symbol);
    }

    functionScope = scopeCreate(scope);
    if (functionScope == NULL) {
        return;
    }

    for (i = 0; i < node->data.func_decl.param_count; i++) {
        AstNode *param = node->data.func_decl.params[i];
        Symbol *paramSymbol = symbolCreateVariable(
            param->data.param.name,
            semanticTypeFromAst(param->data.param.declared_type)
        );

        if (paramSymbol == NULL) {
            continue;
        }

        if (!scopeDeclare(functionScope, paramSymbol)) {
            appendDuplicateDeclarationError(param, result, param->data.param.name);
            symbolFree(paramSymbol);
        }
    }

    if (!node->data.func_decl.is_extern) {
        collectSymbols(node->data.func_decl.body, functionScope, result);
    }
    scopeFree(functionScope);
}

static void collectVariable(AstNode *node, Scope *scope, SemanticResult *result) {
    Symbol *symbol = symbolCreateVariable(
        node->data.var_decl.name,
        semanticTypeFromAst(node->data.var_decl.declared_type)
    );

    if (symbol == NULL) {
        return;
    }

    if (!scopeDeclare(scope, symbol)) {
        appendDuplicateDeclarationError(node, result, node->data.var_decl.name);
        symbolFree(symbol);
    }
}

static void collectSymbols(AstNode *node, Scope *scope, SemanticResult *result) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case AST_PROGRAM: {
            size_t i;
            for (i = 0; i < node->data.program.count; i++) {
                collectSymbols(node->data.program.items[i], scope, result);
            }
            return;
        }
        case AST_BLOCK:
            collectBlock(node, scope, result);
            return;
        case AST_VAR_DECL:
            collectVariable(node, scope, result);
            return;
        case AST_IF:
            collectSymbols(node->data.if_stmt.then_branch, scope, result);
            collectSymbols(node->data.if_stmt.else_branch, scope, result);
            return;
        case AST_WHILE:
            collectSymbols(node->data.while_stmt.body, scope, result);
            return;
        case AST_FOR:
            collectSymbols(node->data.for_stmt.body, scope, result);
            return;
        case AST_FUNC_DECL:
            collectFunction(node, scope, result);
            return;
        default:
            return;
    }
}

static void appendInitializerTypeError(AstNode *node, SemanticResult *result, const char *name, const SemanticType *declaredType, const SemanticType *initializerType) {
    char message[256];

    snprintf(
        message,
        sizeof(message),
        "cannot initialize variable '%s' of type %s with %s",
        name,
        semanticTypeName(declaredType),
        semanticTypeName(initializerType)
    );
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendUndeclaredVariableError(AstNode *node, SemanticResult *result, const char *name) {
    char message[256];

    snprintf(message, sizeof(message), "use of undeclared variable '%s'", name);
    appendCategorizedError(node, result, SEMANTIC_ERROR_NAME, message);
}

static void appendInvalidGlobalInitializerError(AstNode *node, SemanticResult *result, const char *name) {
    char message[256];

    snprintf(message, sizeof(message), "global initializer for '%s' must be a literal", name);
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static bool isAllowedGlobalInitializer(AstNode *node) {
    if (node == NULL) {
        return true;
    }

    switch (node->type) {
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
        case AST_BOOL_LITERAL:
            return true;
        default:
            return false;
    }
}

static void appendNumericOperandError(AstNode *node, SemanticResult *result, AstBinaryOp op) {
    const char *operatorName = "+";

    switch (op) {
        case AST_BINARY_ADD:
            operatorName = "+";
            break;
        case AST_BINARY_SUB:
            operatorName = "-";
            break;
        case AST_BINARY_MUL:
            operatorName = "*";
            break;
        case AST_BINARY_DIV:
            operatorName = "/";
            break;
        default:
            break;
    }

    {
        char message[256];
        snprintf(message, sizeof(message), "operator '%s' requires numeric operands", operatorName);
        appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
    }
}

static void appendConditionTypeError(AstNode *node, SemanticResult *result) {
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "condition must be bool");
}

static bool isNonVoidPointerType(const SemanticType *type) {
    return type != NULL
        && type->kind == SEM_TYPE_POINTER
        && type->element_type != NULL
        && type->element_type->kind != SEM_TYPE_VOID;
}

static SemanticType *checkPointerArithmetic(AstNode *node, SemanticType *leftType, SemanticType *rightType) {
    if (node == NULL) {
        return NULL;
    }

    if ((node->data.binary.op != AST_BINARY_ADD && node->data.binary.op != AST_BINARY_SUB)
            || !isNonVoidPointerType(leftType)
            || rightType == NULL
            || rightType->kind != SEM_TYPE_INT) {
        return NULL;
    }

    return semanticTypeClone(leftType);
}

static void appendAssignmentTypeError(AstNode *node, SemanticResult *result, const SemanticType *expectedType, const SemanticType *actualType) {
    char message[256];

    snprintf(
        message,
        sizeof(message),
        "assignment expects %s but got %s",
        semanticTypeName(expectedType),
        semanticTypeName(actualType)
    );
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendUndeclaredFunctionError(AstNode *node, SemanticResult *result, const char *name) {
    char message[256];

    snprintf(message, sizeof(message), "call to undeclared function '%s'", name);
    appendCategorizedError(node, result, SEMANTIC_ERROR_NAME, message);
}

static void appendWrongArgumentCountError(AstNode *node, SemanticResult *result, const char *name, size_t expected, size_t actual) {
    char message[256];

    snprintf(message, sizeof(message), "function '%s' expects %zu arguments but got %zu", name, expected, actual);
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendWrongArgumentTypeError(AstNode *node, SemanticResult *result, const char *name, size_t index, const SemanticType *expectedType, const SemanticType *actualType) {
    char message[256];

    snprintf(
        message,
        sizeof(message),
        "argument %zu of function '%s' expects %s but got %s",
        index,
        name,
        semanticTypeName(expectedType),
        semanticTypeName(actualType)
    );
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendMissingReturnError(AstNode *node, SemanticResult *result, const char *name, const SemanticType *returnType) {
    char message[256];

    snprintf(message, sizeof(message), "function '%s' with return type %s must return a value", name, semanticTypeName(returnType));
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendIncompatibleReturnTypeError(AstNode *node, SemanticResult *result, const char *name, const SemanticType *expectedType, const SemanticType *actualType) {
    char message[256];

    snprintf(message, sizeof(message), "return type of function '%s' expects %s but got %s", name, semanticTypeName(expectedType), semanticTypeName(actualType));
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendVoidReturnValueError(AstNode *node, SemanticResult *result, const char *name) {
    char message[256];

    snprintf(message, sizeof(message), "function '%s' with return type void cannot return a value", name);
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendEmptyReturnError(AstNode *node, SemanticResult *result, const char *name, const SemanticType *returnType) {
    char message[256];

    snprintf(message, sizeof(message), "function '%s' with return type %s cannot use empty return", name, semanticTypeName(returnType));
    appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
}

static void appendUnreachableCodeError(AstNode *node, SemanticResult *result) {
    appendCategorizedError(node, result, SEMANTIC_ERROR_DEVELOPER, "unreachable code");
}

static bool semanticTypeIsNumeric(const SemanticType *type) {
    return type != NULL && (type->kind == SEM_TYPE_INT || type->kind == SEM_TYPE_FLOAT);
}

static bool semanticTypeIsCompatible(const SemanticType *expected, const SemanticType *actual) {
    if (expected == NULL || actual == NULL) {
        return false;
    }

    if (semanticTypeEquals(expected, actual)) {
        return true;
    }

    if (expected->kind != SEM_TYPE_ARRAY || actual->kind != SEM_TYPE_ARRAY) {
        return false;
    }

    if (expected->has_array_size && actual->has_array_size && expected->array_size != actual->array_size) {
        return false;
    }

    return semanticTypeIsCompatible(expected->element_type, actual->element_type);
}

static bool semanticTypeIsBool(const SemanticType *type) {
    return type != NULL && type->kind == SEM_TYPE_BOOL;
}

static size_t semanticStringLiteralLength(const char *value) {
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

static Scope *createValidationRoot(const Scope *globalScope) {
    Scope *root = scopeCreate(NULL);
    size_t i;

    if (root == NULL || globalScope == NULL) {
        return root;
    }

    for (i = 0; i < globalScope->bucket_count; i++) {
        Symbol *current = globalScope->buckets[i];
        while (current != NULL) {
            if (current->kind == SYMBOL_FUNCTION) {
                Symbol *copy = symbolCreateFunction(current->name, semanticTypeClone(current->type), current->params, current->param_count);
                if (copy != NULL) {
                    scopeDeclare(root, copy);
                }
            }
            current = current->next;
        }
    }

    return root;
}

static void declareVariableInScope(const char *name, AstNode *declaredTypeNode, Scope *scope) {
    Symbol *symbol;

    if (scopeLookupCurrent(scope, name) != NULL) {
        return;
    }

    symbol = symbolCreateVariable(name, semanticTypeFromAst(declaredTypeNode));
    if (symbol == NULL) {
        return;
    }

    if (!scopeDeclare(scope, symbol)) {
        symbolFree(symbol);
    }
}

static SemanticType *checkIdentifier(AstNode *node, Scope *scope, SemanticResult *result) {
    Symbol *symbol = scopeLookup(scope, node->data.identifier.name);

    if (symbol == NULL || symbol->kind != SYMBOL_VARIABLE) {
        appendUndeclaredVariableError(node, result, node->data.identifier.name);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    return semanticTypeClone(symbol->type);
}

static SemanticType *checkAssignmentTarget(AstNode *node, Scope *scope, SemanticContext *ctx) {
    if (node == NULL) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    switch (node->type) {
        case AST_IDENTIFIER: {
            Symbol *targetSymbol = scopeLookup(scope, node->data.identifier.name);

            if (targetSymbol != NULL && targetSymbol->kind == SYMBOL_VARIABLE) {
                return semanticTypeClone(targetSymbol->type);
            }

            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }
        case AST_ARRAY_ACCESS:
            return checkArrayAccess(node, scope, ctx);
        case AST_UNARY: {
            SemanticType *pointerType;

            if (node->data.unary.op != AST_UNARY_DEREF) {
                return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
            }

            pointerType = checkExpression(node->data.unary.operand, scope, ctx);
            if (pointerType->kind == SEM_TYPE_ERROR) {
                return pointerType;
            }

            if (pointerType->kind != SEM_TYPE_POINTER || pointerType->element_type == NULL) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "cannot dereference non-pointer value");
                semanticTypeFree(pointerType);
                return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
            }

            {
                SemanticType *pointeeType = semanticTypeClone(pointerType->element_type);
                semanticTypeFree(pointerType);
                return pointeeType;
            }
        }
        default:
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }
}

static SemanticType *checkAssign(AstNode *node, Scope *scope, SemanticContext *ctx) {
    SemanticType *targetType = checkAssignmentTarget(node->data.assign.target, scope, ctx);
    SemanticType *valueType = checkExpression(node->data.assign.value, scope, ctx);

    if (targetType->kind != SEM_TYPE_ERROR && valueType->kind != SEM_TYPE_ERROR && !semanticTypeIsCompatible(targetType, valueType)) {
        if (node->data.assign.target->type == AST_UNARY && node->data.assign.target->data.unary.op == AST_UNARY_DEREF) {
            appendAssignmentTypeError(node, ctx->result, targetType, valueType);
        } else {
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "assignment types are incompatible");
        }
    }

    semanticTypeFree(valueType);
    return targetType;

}

static SemanticType *checkBinary(AstNode *node, Scope *scope, SemanticContext *ctx) {
    SemanticType *leftType = checkExpression(node->data.binary.left, scope, ctx);
    SemanticType *rightType = checkExpression(node->data.binary.right, scope, ctx);
    SemanticType *resultType = semanticTypeNewPrimitive(SEM_TYPE_ERROR);

    switch (node->data.binary.op) {
        case AST_BINARY_ADD:
        case AST_BINARY_SUB:
        case AST_BINARY_MUL:
        case AST_BINARY_DIV:
            if (semanticTypeIsNumeric(leftType) && semanticTypeIsNumeric(rightType) && semanticTypeEquals(leftType, rightType)) {
                semanticTypeFree(resultType);
                resultType = semanticTypeClone(leftType);
            } else if (node->data.binary.op != AST_BINARY_MUL && node->data.binary.op != AST_BINARY_DIV) {
                SemanticType *pointerResult = checkPointerArithmetic(node, leftType, rightType);

                if (pointerResult != NULL) {
                    semanticTypeFree(resultType);
                    resultType = pointerResult;
                    break;
                }

                appendNumericOperandError(node, ctx->result, node->data.binary.op);
            } else {
                appendNumericOperandError(node, ctx->result, node->data.binary.op);
            }
            break;
        case AST_BINARY_GT:
        case AST_BINARY_LT:
        case AST_BINARY_GTE:
        case AST_BINARY_LTE:
            if (!semanticTypeIsNumeric(leftType) || !semanticTypeIsNumeric(rightType) || !semanticTypeEquals(leftType, rightType)) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "comparison requires compatible numeric operands");
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeNewPrimitive(SEM_TYPE_BOOL);
            }
            break;
        case AST_BINARY_EQ:
        case AST_BINARY_NEQ:
            if (!semanticTypeEquals(leftType, rightType)) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "comparison requires compatible operands");
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeNewPrimitive(SEM_TYPE_BOOL);
            }
            break;
        case AST_BINARY_AND:
        case AST_BINARY_OR:
            if (!semanticTypeIsBool(leftType) || !semanticTypeIsBool(rightType)) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "logical operators require bool operands");
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeNewPrimitive(SEM_TYPE_BOOL);
            }
            break;
    }

    semanticTypeFree(leftType);
    semanticTypeFree(rightType);
    return resultType;
}

static SemanticType *checkUnary(AstNode *node, Scope *scope, SemanticContext *ctx) {
    SemanticType *operandType;

    if (node->data.unary.op == AST_UNARY_ADDRESS_OF) {
        SemanticType *pointerType;

        if (!isAddressableExpression(node->data.unary.operand)) {
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "address-of requires an addressable expression");
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }

        operandType = checkAssignmentTarget(node->data.unary.operand, scope, ctx);
        if (operandType->kind == SEM_TYPE_ERROR) {
            return operandType;
        }

        pointerType = semanticTypeNewPrimitive(SEM_TYPE_POINTER);
        if (pointerType == NULL) {
            semanticTypeFree(operandType);
            return NULL;
        }

        pointerType->element_type = operandType;
        return pointerType;
    }

    operandType = checkExpression(node->data.unary.operand, scope, ctx);

    if (node->data.unary.op == AST_UNARY_DEREF) {
        SemanticType *pointeeType;

        if (operandType->kind != SEM_TYPE_POINTER || operandType->element_type == NULL) {
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "cannot dereference non-pointer value");
            semanticTypeFree(operandType);
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }

        pointeeType = semanticTypeClone(operandType->element_type);
        semanticTypeFree(operandType);
        return pointeeType;
    }

    if (node->data.unary.op == AST_UNARY_NOT) {
        if (!semanticTypeIsBool(operandType)) {
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "operator '!' requires bool operand");
            semanticTypeFree(operandType);
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }

        semanticTypeFree(operandType);
        return semanticTypeNewPrimitive(SEM_TYPE_BOOL);
    }

    if (!semanticTypeIsNumeric(operandType)) {
        appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "operator '-' requires numeric operand");
        semanticTypeFree(operandType);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    return operandType;
}

static SemanticType *checkArrayLiteral(AstNode *node, Scope *scope, SemanticContext *ctx) {
    SemanticType *arrayType = semanticTypeNewPrimitive(SEM_TYPE_ARRAY);
    SemanticType *elementType = NULL;
    size_t i;

    if (arrayType == NULL) {
        return NULL;
    }

    if (node->data.array_literal.count == 0) {
        arrayType->element_type = semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        return arrayType;
    }

    for (i = 0; i < node->data.array_literal.count; i++) {
        SemanticType *currentType = checkExpression(node->data.array_literal.elements[i], scope, ctx);

        if (elementType == NULL) {
            elementType = semanticTypeClone(currentType);
        } else if (currentType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(elementType, currentType)) {
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "array literal elements must have compatible types");
            semanticTypeFree(elementType);
            elementType = semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }

        semanticTypeFree(currentType);
    }

    if (elementType == NULL) {
        elementType = semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    arrayType->element_type = elementType;
    return arrayType;
}

static SemanticType *checkArrayAccess(AstNode *node, Scope *scope, SemanticContext *ctx) {
    SemanticType *baseType = checkExpression(node->data.array_access.base, scope, ctx);
    size_t i;

    for (i = 0; i < node->data.array_access.index_count; i++) {
        SemanticType *indexType = checkExpression(node->data.array_access.indices[i], scope, ctx);

        if (indexType->kind != SEM_TYPE_ERROR && indexType->kind != SEM_TYPE_INT) {
            appendCategorizedError(node->data.array_access.indices[i], ctx->result, SEMANTIC_ERROR_TYPE, "array index must be int");
        }

        if (baseType->kind != SEM_TYPE_ARRAY) {
            char message[256];

            snprintf(message, sizeof(message), "cannot index non-array value of type %s", semanticTypeName(baseType));
            appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, message);
            semanticTypeFree(indexType);
            semanticTypeFree(baseType);
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
        }

        {
            SemanticType *nextType = semanticTypeClone(baseType->element_type);
            semanticTypeFree(baseType);
            baseType = nextType;
        }

        semanticTypeFree(indexType);
    }

    return baseType;
}

static SemanticType *checkCall(AstNode *node, Scope *scope, SemanticContext *ctx) {
    Symbol *callee;
    size_t i;

    if (node->data.call.callee->type != AST_IDENTIFIER) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    callee = scopeLookup(scope, node->data.call.callee->data.identifier.name);
    if (callee == NULL || callee->kind != SYMBOL_FUNCTION) {
        appendUndeclaredFunctionError(node->data.call.callee, ctx->result, node->data.call.callee->data.identifier.name);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    if (node->data.call.arg_count != callee->param_count) {
        appendWrongArgumentCountError(node->data.call.callee, ctx->result, callee->name, callee->param_count, node->data.call.arg_count);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    for (i = 0; i < node->data.call.arg_count; i++) {
        SemanticType *argType = checkExpression(node->data.call.args[i], scope, ctx);
        if (argType->kind != SEM_TYPE_ERROR && !semanticTypeIsCompatible(callee->params[i], argType)) {
            appendWrongArgumentTypeError(node->data.call.args[i], ctx->result, callee->name, i + 1, callee->params[i], argType);
        }
        semanticTypeFree(argType);
    }

    return semanticTypeClone(callee->type);
}

static SemanticType *checkExpression(AstNode *node, Scope *scope, SemanticContext *ctx) {
    
    if (node == NULL) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    switch (node->type) {
        case AST_IDENTIFIER:
            return checkIdentifier(node, scope, ctx->result);
        case AST_INT_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_INT);
        case AST_FLOAT_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_FLOAT);
        case AST_STRING_LITERAL:
            if (semanticStringLiteralLength(node->data.string_literal.value) == 1) {
                return semanticTypeNewPrimitive(SEM_TYPE_CHAR);
            }
            return semanticTypeNewPrimitive(SEM_TYPE_STRING);
        case AST_BOOL_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_BOOL);
        case AST_ARRAY_LITERAL:
            return checkArrayLiteral(node, scope, ctx);
        case AST_ARRAY_ACCESS:
            return checkArrayAccess(node, scope, ctx);
        case AST_BINARY:
            return checkBinary(node, scope, ctx);
        case AST_UNARY:
            return checkUnary(node, scope, ctx);
        case AST_CALL:
            return checkCall(node, scope, ctx);
        case AST_ASSIGN:
            return checkAssign(node, scope, ctx);
        default:
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }
}

static void validateVariableDeclaration(AstNode *node, Scope *scope, SemanticContext *ctx, bool requireLiteralInitializer) {
    SemanticType *declaredType = semanticTypeFromAst(node->data.var_decl.declared_type);

    if (node->data.var_decl.initializer != NULL) {
        if (requireLiteralInitializer && !isAllowedGlobalInitializer(node->data.var_decl.initializer)) {
            appendInvalidGlobalInitializerError(node, ctx->result, node->data.var_decl.name);
        } else {
            SemanticType *initializerType = checkExpression(node->data.var_decl.initializer, scope, ctx);
            if (initializerType->kind != SEM_TYPE_ERROR && !semanticTypeIsCompatible(declaredType, initializerType)) {
                appendInitializerTypeError(node, ctx->result, node->data.var_decl.name, declaredType, initializerType);
            }
            semanticTypeFree(initializerType);
        }
    }

    semanticTypeFree(declaredType);
    declareVariableInScope(node->data.var_decl.name, node->data.var_decl.declared_type, scope);
}

static FlowState validateBlock(AstNode *node, Scope *scope, SemanticContext *ctx) {
    Scope *blockScope = scopeCreate(scope);
    FlowState flow = FLOW_CONTINUES;
    size_t i;

    if (blockScope == NULL) {
        return FLOW_CONTINUES;
    }

    for (i = 0; i < node->data.block.count; i++) {
        AstNode *item = node->data.block.items[i];

        if (flow == FLOW_RETURNS) {
            appendUnreachableCodeError(item, ctx->result);
            validateNode(item, blockScope, ctx);
            continue;
        }

        flow = validateNode(item, blockScope, ctx);
    }

    scopeFree(blockScope);
    return flow;
}

static void validateFunction(AstNode *node, Scope *scope, SemanticContext *ctx) {
    Scope *functionScope = scopeCreate(scope);
    FunctionContext context;
    FunctionContext *previousContext = ctx->current_function;
    FlowState flow;
    size_t i;

    if (functionScope == NULL) {
        return;
    }

    if (node->data.func_decl.is_extern) {
        scopeFree(functionScope);
        return;
    }

    context.function_name = node->data.func_decl.name;
    context.return_type = semanticTypeFromAst(node->data.func_decl.return_type);
    ctx->current_function = &context;

    for (i = 0; i < node->data.func_decl.param_count; i++) {
        AstNode *param = node->data.func_decl.params[i];
        declareVariableInScope(param->data.param.name, param->data.param.declared_type, functionScope);
    }

    flow = validateNode(node->data.func_decl.body, functionScope, ctx);
    if (context.return_type != NULL && context.return_type->kind != SEM_TYPE_VOID && flow != FLOW_RETURNS) {
        appendMissingReturnError(node, ctx->result, node->data.func_decl.name, context.return_type);
    }
    semanticTypeFree(context.return_type);
    ctx->current_function = previousContext;
    scopeFree(functionScope);
}

static FlowState validateNode(AstNode *node, Scope *scope, SemanticContext *ctx) {
    size_t i;

    if (node == NULL) {
        return FLOW_CONTINUES;
    }

    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++) {
                AstNode *item = node->data.program.items[i];

                if (item != NULL && item->type == AST_VAR_DECL) {
                    validateVariableDeclaration(item, scope, ctx, true);
                    continue;
                }

                validateNode(item, scope, ctx);
            }
            return FLOW_CONTINUES;
        case AST_BLOCK:
            return validateBlock(node, scope, ctx);
        case AST_VAR_DECL:
            validateVariableDeclaration(node, scope, ctx, false);
            return FLOW_CONTINUES;
        case AST_EXPR_STMT:
        case AST_ASSIGN: {
            SemanticType *exprType = checkExpression(node->type == AST_EXPR_STMT ? node->data.expr_stmt.expression : node, scope, ctx);
            semanticTypeFree(exprType);
            return FLOW_CONTINUES;
        }
        case AST_IF: {
            FlowState thenFlow;
            FlowState elseFlow;
            SemanticType *conditionType = checkExpression(node->data.if_stmt.condition, scope, ctx);
            if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                appendConditionTypeError(node->data.if_stmt.condition, ctx->result);
            }
            semanticTypeFree(conditionType);
            thenFlow = validateNode(node->data.if_stmt.then_branch, scope, ctx);
            if (node->data.if_stmt.else_branch == NULL) {
                return FLOW_CONTINUES;
            }

            elseFlow = validateNode(node->data.if_stmt.else_branch, scope, ctx);
            if (thenFlow == FLOW_RETURNS && elseFlow == FLOW_RETURNS) {
                return FLOW_RETURNS;
            }

            return FLOW_CONTINUES;
        }
        case AST_WHILE: {
            SemanticType *conditionType = checkExpression(node->data.while_stmt.condition, scope, ctx);
            if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                appendConditionTypeError(node->data.while_stmt.condition, ctx->result);
            }
            semanticTypeFree(conditionType);
            ctx->loop_depth++;
            validateNode(node->data.while_stmt.body, scope, ctx);
            ctx->loop_depth--;
            return FLOW_CONTINUES;
        }
        case AST_FOR: {
            Scope *forScope = scopeCreate(scope);
            if (forScope == NULL) {
                return FLOW_CONTINUES;
            }

            if (node->data.for_stmt.init != NULL) {
                validateNode(node->data.for_stmt.init, forScope, ctx);
            }
            if (node->data.for_stmt.condition != NULL) {
                SemanticType *conditionType = checkExpression(node->data.for_stmt.condition, forScope, ctx);
                if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                    appendConditionTypeError(node->data.for_stmt.condition, ctx->result);
                }
                semanticTypeFree(conditionType);
            }
            if (node->data.for_stmt.update != NULL) {
                SemanticType *updateType = checkExpression(node->data.for_stmt.update, forScope, ctx);
                semanticTypeFree(updateType);
            }
            ctx->loop_depth++;
            validateNode(node->data.for_stmt.body, forScope, ctx);
            ctx->loop_depth--;
            scopeFree(forScope);
            return FLOW_CONTINUES;
        }
        case AST_FUNC_DECL:
            validateFunction(node, scope, ctx);
            return FLOW_CONTINUES;
        case AST_RETURN:
            if (ctx->current_function == NULL || ctx->current_function->return_type == NULL) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_TYPE, "return statement outside function");
                return FLOW_RETURNS;
            }

            if (node->data.return_stmt.value == NULL) {
                if (ctx->current_function->return_type->kind != SEM_TYPE_VOID) {
                    appendEmptyReturnError(node, ctx->result, ctx->current_function->function_name, ctx->current_function->return_type);
                }
                return FLOW_RETURNS;
            }

            if (node->data.return_stmt.value != NULL) {
                SemanticType *returnType = checkExpression(node->data.return_stmt.value, scope, ctx);

                if (ctx->current_function->return_type->kind == SEM_TYPE_VOID) {
                    appendVoidReturnValueError(node, ctx->result, ctx->current_function->function_name);
                }

                if (ctx->current_function->return_type->kind != SEM_TYPE_VOID && returnType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(ctx->current_function->return_type, returnType)) {
                    appendIncompatibleReturnTypeError(node, ctx->result, ctx->current_function->function_name, ctx->current_function->return_type, returnType);
                }

                semanticTypeFree(returnType);
            }
            return FLOW_RETURNS;
        case AST_BREAK:
            if (ctx->loop_depth == 0) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_DEVELOPER, "break statement outside loop");
            }
            return FLOW_CONTINUES;
        case AST_CONTINUE:
            if (ctx->loop_depth == 0) {
                appendCategorizedError(node, ctx->result, SEMANTIC_ERROR_DEVELOPER, "continue statement outside loop");
            }
            return FLOW_CONTINUES;
        default:
            return FLOW_CONTINUES;
    }
}

SemanticResult semanticAnalyze(AstNode *program) {
    SemanticResult result;
    SemanticContext ctx;
    Scope *validationRoot;

    semanticErrorListInit(&result.errors);
    result.global_scope = scopeCreate(NULL);
    ctx.result = &result;
    ctx.current_function = NULL;
    ctx.loop_depth = 0;
    if (result.global_scope != NULL) {
        collectSymbols(program, result.global_scope, &result);
        validationRoot = createValidationRoot(result.global_scope);
        if (validationRoot != NULL) {
            validateNode(program, validationRoot, &ctx);
            scopeFree(validationRoot);
        }
    }
    return result;
}

void semanticResultFree(SemanticResult *result) {
    scopeFree(result->global_scope);
    result->global_scope = NULL;
    semanticErrorListFree(&result->errors);
}
