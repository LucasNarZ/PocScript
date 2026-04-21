#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>

static void collectSymbols(AstNode *node, Scope *scope, SemanticResult *result);
static void validateNode(AstNode *node, Scope *scope, SemanticResult *result);
static SemanticType *checkExpression(AstNode *node, Scope *scope, SemanticResult *result);

typedef struct {
    const char *function_name;
    SemanticType *return_type;
    bool found_compatible_return;
} FunctionContext;

static FunctionContext *currentFunctionContext = NULL;

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

    collectSymbols(node->data.func_decl.body, functionScope, result);
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

static bool semanticTypeIsNumeric(const SemanticType *type) {
    return type != NULL && (type->kind == SEM_TYPE_INT || type->kind == SEM_TYPE_FLOAT);
}

static bool semanticTypeIsBool(const SemanticType *type) {
    return type != NULL && type->kind == SEM_TYPE_BOOL;
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

static SemanticType *checkAssign(AstNode *node, Scope *scope, SemanticResult *result) {
    SemanticType *valueType = checkExpression(node->data.assign.value, scope, result);

    if (node->data.assign.target->type == AST_IDENTIFIER) {
        Symbol *targetSymbol = scopeLookup(scope, node->data.assign.target->data.identifier.name);

        if (targetSymbol != NULL && targetSymbol->kind == SYMBOL_VARIABLE) {
            if (valueType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(targetSymbol->type, valueType)) {
                appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "assignment types are incompatible");
            }
            semanticTypeFree(valueType);
            return semanticTypeClone(targetSymbol->type);
        }

        return valueType;
    }

    return valueType;
}

static SemanticType *checkBinary(AstNode *node, Scope *scope, SemanticResult *result) {
    SemanticType *leftType = checkExpression(node->data.binary.left, scope, result);
    SemanticType *rightType = checkExpression(node->data.binary.right, scope, result);
    SemanticType *resultType = semanticTypeNewPrimitive(SEM_TYPE_ERROR);

    switch (node->data.binary.op) {
        case AST_BINARY_ADD:
        case AST_BINARY_SUB:
        case AST_BINARY_MUL:
        case AST_BINARY_DIV:
            if (!semanticTypeIsNumeric(leftType) || !semanticTypeIsNumeric(rightType) || !semanticTypeEquals(leftType, rightType)) {
                appendNumericOperandError(node, result, node->data.binary.op);
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeClone(leftType);
            }
            break;
        case AST_BINARY_GT:
        case AST_BINARY_LT:
        case AST_BINARY_GTE:
        case AST_BINARY_LTE:
            if (!semanticTypeIsNumeric(leftType) || !semanticTypeIsNumeric(rightType) || !semanticTypeEquals(leftType, rightType)) {
                appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "comparison requires compatible numeric operands");
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeNewPrimitive(SEM_TYPE_BOOL);
            }
            break;
        case AST_BINARY_EQ:
        case AST_BINARY_NEQ:
            if (!semanticTypeEquals(leftType, rightType)) {
                appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "comparison requires compatible operands");
            } else {
                semanticTypeFree(resultType);
                resultType = semanticTypeNewPrimitive(SEM_TYPE_BOOL);
            }
            break;
        case AST_BINARY_AND:
        case AST_BINARY_OR:
            if (!semanticTypeIsBool(leftType) || !semanticTypeIsBool(rightType)) {
                appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "logical operators require bool operands");
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

static SemanticType *checkUnary(AstNode *node, Scope *scope, SemanticResult *result) {
    SemanticType *operandType = checkExpression(node->data.unary.operand, scope, result);

    if (!semanticTypeIsBool(operandType)) {
        appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "operator '!' requires bool operand");
        semanticTypeFree(operandType);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    semanticTypeFree(operandType);
    return semanticTypeNewPrimitive(SEM_TYPE_BOOL);
}

static SemanticType *checkArrayLiteral(AstNode *node, Scope *scope, SemanticResult *result) {
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
        SemanticType *currentType = checkExpression(node->data.array_literal.elements[i], scope, result);

        if (elementType == NULL) {
            elementType = semanticTypeClone(currentType);
        } else if (currentType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(elementType, currentType)) {
            appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "array literal elements must have compatible types");
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

static SemanticType *checkArrayAccess(AstNode *node, Scope *scope, SemanticResult *result) {
    SemanticType *baseType = checkExpression(node->data.array_access.base, scope, result);
    size_t i;

    for (i = 0; i < node->data.array_access.index_count; i++) {
        SemanticType *indexType = checkExpression(node->data.array_access.indices[i], scope, result);

        if (indexType->kind != SEM_TYPE_ERROR && indexType->kind != SEM_TYPE_INT) {
            appendCategorizedError(node->data.array_access.indices[i], result, SEMANTIC_ERROR_TYPE, "array index must be int");
        }

        if (baseType->kind != SEM_TYPE_ARRAY) {
            char message[256];

            snprintf(message, sizeof(message), "cannot index non-array value of type %s", semanticTypeName(baseType));
            appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, message);
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

static SemanticType *checkCall(AstNode *node, Scope *scope, SemanticResult *result) {
    Symbol *callee;
    size_t i;

    if (node->data.call.callee->type != AST_IDENTIFIER) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    callee = scopeLookup(scope, node->data.call.callee->data.identifier.name);
    if (callee == NULL || callee->kind != SYMBOL_FUNCTION) {
        appendUndeclaredFunctionError(node->data.call.callee, result, node->data.call.callee->data.identifier.name);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    if (node->data.call.arg_count != callee->param_count) {
        appendWrongArgumentCountError(node->data.call.callee, result, callee->name, callee->param_count, node->data.call.arg_count);
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    for (i = 0; i < node->data.call.arg_count; i++) {
        SemanticType *argType = checkExpression(node->data.call.args[i], scope, result);
        if (argType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(callee->params[i], argType)) {
            appendWrongArgumentTypeError(node->data.call.args[i], result, callee->name, i + 1, callee->params[i], argType);
        }
        semanticTypeFree(argType);
    }

    return semanticTypeClone(callee->type);
}

static SemanticType *checkExpression(AstNode *node, Scope *scope, SemanticResult *result) {
    
    if (node == NULL) {
        return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }

    switch (node->type) {
        case AST_IDENTIFIER:
            return checkIdentifier(node, scope, result);
        case AST_INT_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_INT);
        case AST_FLOAT_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_FLOAT);
        case AST_STRING_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_CHAR);
        case AST_BOOL_LITERAL:
            return semanticTypeNewPrimitive(SEM_TYPE_BOOL);
        case AST_ARRAY_LITERAL:
            return checkArrayLiteral(node, scope, result);
        case AST_ARRAY_ACCESS:
            return checkArrayAccess(node, scope, result);
        case AST_BINARY:
            return checkBinary(node, scope, result);
        case AST_UNARY:
            return checkUnary(node, scope, result);
        case AST_CALL:
            return checkCall(node, scope, result);
        case AST_ASSIGN:
            return checkAssign(node, scope, result);
        default:
            return semanticTypeNewPrimitive(SEM_TYPE_ERROR);
    }
}

static void validateBlock(AstNode *node, Scope *scope, SemanticResult *result) {
    Scope *blockScope = scopeCreate(scope);
    size_t i;

    if (blockScope == NULL) {
        return;
    }

    for (i = 0; i < node->data.block.count; i++) {
        validateNode(node->data.block.items[i], blockScope, result);
    }

    scopeFree(blockScope);
}

static void validateFunction(AstNode *node, Scope *scope, SemanticResult *result) {
    Scope *functionScope = scopeCreate(scope);
    FunctionContext context;
    FunctionContext *previousContext = currentFunctionContext;
    size_t i;

    if (functionScope == NULL) {
        return;
    }

    context.function_name = node->data.func_decl.name;
    context.return_type = semanticTypeFromAst(node->data.func_decl.return_type);
    context.found_compatible_return = false;
    currentFunctionContext = &context;

    for (i = 0; i < node->data.func_decl.param_count; i++) {
        AstNode *param = node->data.func_decl.params[i];
        declareVariableInScope(param->data.param.name, param->data.param.declared_type, functionScope);
    }

    validateNode(node->data.func_decl.body, functionScope, result);
    if (context.return_type != NULL && context.return_type->kind != SEM_TYPE_VOID && !context.found_compatible_return) {
        appendMissingReturnError(node, result, node->data.func_decl.name, context.return_type);
    }
    semanticTypeFree(context.return_type);
    currentFunctionContext = previousContext;
    scopeFree(functionScope);
}

static void validateNode(AstNode *node, Scope *scope, SemanticResult *result) {
    size_t i;

    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case AST_PROGRAM:
            for (i = 0; i < node->data.program.count; i++) {
                validateNode(node->data.program.items[i], scope, result);
            }
            return;
        case AST_BLOCK:
            validateBlock(node, scope, result);
            return;
        case AST_VAR_DECL: {
            SemanticType *declaredType = semanticTypeFromAst(node->data.var_decl.declared_type);

            if (node->data.var_decl.initializer != NULL) {
                SemanticType *initializerType = checkExpression(node->data.var_decl.initializer, scope, result);
                if (initializerType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(declaredType, initializerType)) {
                    appendInitializerTypeError(node, result, node->data.var_decl.name, declaredType, initializerType);
                }
                semanticTypeFree(initializerType);
            }

            semanticTypeFree(declaredType);
            declareVariableInScope(node->data.var_decl.name, node->data.var_decl.declared_type, scope);
            return;
        }
        case AST_EXPR_STMT:
        case AST_ASSIGN: {
            SemanticType *exprType = checkExpression(node->type == AST_EXPR_STMT ? node->data.expr_stmt.expression : node, scope, result);
            semanticTypeFree(exprType);
            return;
        }
        case AST_IF: {
            SemanticType *conditionType = checkExpression(node->data.if_stmt.condition, scope, result);
            if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                appendConditionTypeError(node->data.if_stmt.condition, result);
            }
            semanticTypeFree(conditionType);
            validateNode(node->data.if_stmt.then_branch, scope, result);
            validateNode(node->data.if_stmt.else_branch, scope, result);
            return;
        }
        case AST_WHILE: {
            SemanticType *conditionType = checkExpression(node->data.while_stmt.condition, scope, result);
            if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                appendConditionTypeError(node->data.while_stmt.condition, result);
            }
            semanticTypeFree(conditionType);
            validateNode(node->data.while_stmt.body, scope, result);
            return;
        }
        case AST_FOR: {
            Scope *forScope = scopeCreate(scope);
            if (forScope == NULL) {
                return;
            }

            if (node->data.for_stmt.init != NULL) {
                validateNode(node->data.for_stmt.init, forScope, result);
            }
            if (node->data.for_stmt.condition != NULL) {
                SemanticType *conditionType = checkExpression(node->data.for_stmt.condition, forScope, result);
                if (conditionType->kind != SEM_TYPE_ERROR && !semanticTypeIsBool(conditionType)) {
                    appendConditionTypeError(node->data.for_stmt.condition, result);
                }
                semanticTypeFree(conditionType);
            }
            if (node->data.for_stmt.update != NULL) {
                SemanticType *updateType = checkExpression(node->data.for_stmt.update, forScope, result);
                semanticTypeFree(updateType);
            }
            validateNode(node->data.for_stmt.body, forScope, result);
            scopeFree(forScope);
            return;
        }
        case AST_FUNC_DECL:
            validateFunction(node, scope, result);
            return;
        case AST_RETURN:
            if (currentFunctionContext == NULL || currentFunctionContext->return_type == NULL) {
                appendCategorizedError(node, result, SEMANTIC_ERROR_TYPE, "return statement outside function");
                return;
            }

            if (node->data.return_stmt.value == NULL) {
                if (currentFunctionContext->return_type->kind != SEM_TYPE_VOID) {
                    appendEmptyReturnError(node, result, currentFunctionContext->function_name, currentFunctionContext->return_type);
                    currentFunctionContext->found_compatible_return = true;
                }
                return;
            }

            if (node->data.return_stmt.value != NULL) {
                SemanticType *returnType = checkExpression(node->data.return_stmt.value, scope, result);

                if (currentFunctionContext->return_type->kind == SEM_TYPE_VOID) {
                    appendVoidReturnValueError(node, result, currentFunctionContext->function_name);
                } else {
                    currentFunctionContext->found_compatible_return = true;
                }

                if (currentFunctionContext->return_type->kind != SEM_TYPE_VOID && returnType->kind != SEM_TYPE_ERROR && !semanticTypeEquals(currentFunctionContext->return_type, returnType)) {
                    appendIncompatibleReturnTypeError(node, result, currentFunctionContext->function_name, currentFunctionContext->return_type, returnType);
                }

                semanticTypeFree(returnType);
            }
            return;
        default:
            return;
    }
}

SemanticResult semanticAnalyze(AstNode *program) {
    SemanticResult result;
    Scope *validationRoot;

    semanticErrorListInit(&result.errors);
    result.global_scope = scopeCreate(NULL);
    if (result.global_scope != NULL) {
        collectSymbols(program, result.global_scope, &result);
        validationRoot = createValidationRoot(result.global_scope);
        if (validationRoot != NULL) {
            validateNode(program, validationRoot, &result);
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
