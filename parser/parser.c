#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static bool isTokenType(const Token *token, TokenType type) {
    return token != NULL && token->type == type;
}

static bool isAtEnd(const Token *token) {
   return token == NULL || token->type == TOKEN_EOF;
}

static void syntaxError(const Token *token, const char *message) {
    if (token == NULL) {
        fprintf(stderr, "SyntaxError at end of input: %s\n", message);
        exit(1);
    }

    fprintf(stderr, "SyntaxError at line %d, column %d: %s\n", token->line, token->column, message);
    exit(1);
}

static void expectToken(Token **token, TokenType type, const char *message) {
    if (!isTokenType(*token, type)) {
        syntaxError(*token, message);
    }

    *token = (*token)->next;
}

static bool isTypeToken(const Token *token) {
    return isTokenType(token, TOKEN_TYPE_ARRAY) || isTokenType(token, TOKEN_TYPE_INT) || isTokenType(token, TOKEN_TYPE_FLOAT) || isTokenType(token, TOKEN_TYPE_CHAR) || isTokenType(token, TOKEN_TYPE_BOOL) || isTokenType(token, TOKEN_IDENTIFIER);
}

static bool isDeclarationStart(const Token *token) {
    if (token == NULL || token->next == NULL || token->next->next == NULL) {
        return false;
    }

    return isTokenType(token, TOKEN_IDENTIFIER) && isTokenType(token->next, TOKEN_DOUBLE_COLON) && isTypeToken(token->next->next);
}

static AstBinaryOp binaryOpFromToken(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return AST_BINARY_ADD;
        case TOKEN_MINUS: return AST_BINARY_SUB;
        case TOKEN_STAR: return AST_BINARY_MUL;
        case TOKEN_SLASH: return AST_BINARY_DIV;
        case TOKEN_GT: return AST_BINARY_GT;
        case TOKEN_LT: return AST_BINARY_LT;
        case TOKEN_GT_EQ: return AST_BINARY_GTE;
        case TOKEN_LT_EQ: return AST_BINARY_LTE;
        case TOKEN_EQ_EQ: return AST_BINARY_EQ;
        case TOKEN_NOT_EQ: return AST_BINARY_NEQ;
        case TOKEN_AND_AND: return AST_BINARY_AND;
        case TOKEN_OR_OR: return AST_BINARY_OR;
        default: return AST_BINARY_ADD;
    }
}

static AstAssignOp assignOpFromToken(TokenType type) {
    switch (type) {
        case TOKEN_PLUS_ASSIGN: return AST_ASSIGN_ADD;
        case TOKEN_MINUS_ASSIGN: return AST_ASSIGN_SUB;
        case TOKEN_ASSIGN:
        default:
            return AST_ASSIGN_SET;
    }
}

static AstTypeKind typeKindFromToken(const Token *token) {
    switch (token->type) {
        case TOKEN_TYPE_INT: return AST_TYPE_INT_KIND;
        case TOKEN_TYPE_FLOAT: return AST_TYPE_FLOAT_KIND;
        case TOKEN_TYPE_CHAR: return AST_TYPE_CHAR_KIND;
        case TOKEN_TYPE_BOOL: return AST_TYPE_BOOL_KIND;
        case TOKEN_TYPE_ARRAY: return AST_TYPE_ARRAY_KIND;
        case TOKEN_IDENTIFIER:
        default:
            return AST_TYPE_CUSTOM_KIND;
    }
}

static AstNode *wrapArrayTypes(AstNode *base, Token **token) {
    while (isTokenType(*token, TOKEN_LBRACKET)) {
        AstNode *arrayType = astNewNode(AST_TYPE_ARRAY);
        AstNode *sizeExpr = NULL;

        *token = (*token)->next;
        if (!isTokenType(*token, TOKEN_RBRACKET)) {
            sizeExpr = parseLogical(token);
        }
        expectToken(token, TOKEN_RBRACKET, "expected ']' after array type size");

        arrayType->data.type_array.element_type = base;
        arrayType->data.type_array.size_expr = sizeExpr;
        base = arrayType;
    }

    return base;
}

static AstNode *parseIf(Token **token) {
    AstNode *node = astNewNode(AST_IF);

    *token = (*token)->next;
    expectToken(token, TOKEN_LPAREN, "expected '(' after if");
    node->data.if_stmt.condition = parseLogical(token);
    expectToken(token, TOKEN_RPAREN, "expected ')' after if condition");
    node->data.if_stmt.then_branch = parseBlock(token);

    if (isTokenType(*token, TOKEN_KW_ELSE)) {
        *token = (*token)->next;
        if (isTokenType(*token, TOKEN_KW_IF)) {
            node->data.if_stmt.else_branch = parseIf(token);
        } else {
            node->data.if_stmt.else_branch = parseBlock(token);
        }
    }

    return node;
}

static AstNode *parseWhile(Token **token) {
    AstNode *node = astNewNode(AST_WHILE);

    *token = (*token)->next;
    expectToken(token, TOKEN_LPAREN, "expected '(' after while");
    node->data.while_stmt.condition = parseLogical(token);
    expectToken(token, TOKEN_RPAREN, "expected ')' after while condition");
    node->data.while_stmt.body = parseBlock(token);
    return node;
}

static AstNode *parseFor(Token **token) {
    AstNode *node = astNewNode(AST_FOR);

    *token = (*token)->next;
    expectToken(token, TOKEN_LPAREN, "expected '(' after for");
    if (!isTokenType(*token, TOKEN_SEMICOLON)) {
        node->data.for_stmt.init = parseAssign(token);
    }
    expectToken(token, TOKEN_SEMICOLON, "expected ';' after for init");
    if (!isTokenType(*token, TOKEN_SEMICOLON)) {
        node->data.for_stmt.condition = parseLogical(token);
    }
    expectToken(token, TOKEN_SEMICOLON, "expected ';' after for condition");
    if (!isTokenType(*token, TOKEN_RPAREN)) {
        node->data.for_stmt.update = parseAssign(token);
    }
    expectToken(token, TOKEN_RPAREN, "expected ')' after for update");
    node->data.for_stmt.body = parseBlock(token);
    return node;
}

static AstNode *parseParameter(Token **token) {
    AstNode *param = astNewNode(AST_PARAM);

    if (!isTokenType(*token, TOKEN_IDENTIFIER)) {
        syntaxError(*token, "expected parameter name");
    }

    param->data.param.name = (*token)->value;
    *token = (*token)->next;
    expectToken(token, TOKEN_DOUBLE_COLON, "expected '::' after parameter name");
    param->data.param.declared_type = parseType(token);
    return param;
}

static AstNode *parseFunction(Token **token) {
    AstNode *node = astNewNode(AST_FUNC_DECL);

    *token = (*token)->next;
    if (!isTokenType(*token, TOKEN_IDENTIFIER)) {
        syntaxError(*token, "expected function name after func");
    }

    node->data.func_decl.name = (*token)->value;
    *token = (*token)->next;
    expectToken(token, TOKEN_LPAREN, "expected '(' after function name");

    while (!isTokenType(*token, TOKEN_RPAREN)) {
        AstNode *param = parseParameter(token);
        astAppendNode(&node->data.func_decl.params, &node->data.func_decl.param_count, param);
        if (isTokenType(*token, TOKEN_COMMA)) {
            *token = (*token)->next;
        }
    }

    expectToken(token, TOKEN_RPAREN, "expected ')' after function parameters");
    node->data.func_decl.body = parseBlock(token);
    return node;
}

AstNode *parseProgram(Token **token) {
    AstNode *program = astNewNode(AST_PROGRAM);

    while (!isAtEnd(*token)) {
        AstNode *item = parseStatement(token);
        if (item != NULL) {
            astAppendNode(&program->data.program.items, &program->data.program.count, item);
        }
    }

    return program;
}

AstNode *parseBlock(Token **token) {
    AstNode *block = astNewNode(AST_BLOCK);

    if (isTokenType(*token, TOKEN_LBRACE)) {
        *token = (*token)->next;
    }

    while (!isAtEnd(*token) && !isTokenType(*token, TOKEN_RBRACE)) {
        AstNode *item = parseStatement(token);
        if (item != NULL) {
            astAppendNode(&block->data.block.items, &block->data.block.count, item);
        }
    }

    if (isTokenType(*token, TOKEN_RBRACE)) {
        *token = (*token)->next;
    }

    return block;
}

AstNode *parseStatement(Token **token) {
    AstNode *node;

    if (isAtEnd(*token)) {
        return NULL;
    }

    if (isTokenType(*token, TOKEN_KW_IF)) {
        return parseIf(token);
    }
    if (isTokenType(*token, TOKEN_KW_WHILE)) {
        return parseWhile(token);
    }
    if (isTokenType(*token, TOKEN_KW_FOR)) {
        return parseFor(token);
    }
    if (isTokenType(*token, TOKEN_KW_FUNC)) {
        return parseFunction(token);
    }

    node = parseReturn(token);
    expectToken(token, TOKEN_SEMICOLON, "expected ';' after statement");

    if (node->type == AST_ASSIGN || node->type == AST_VAR_DECL || node->type == AST_RETURN) {
        return node;
    }

    {
        AstNode *exprStmt = astNewNode(AST_EXPR_STMT);
        exprStmt->data.expr_stmt.expression = node;
        return exprStmt;
    }
}

AstNode *parseReturn(Token **token) {
    AstNode *node;

    if (isTokenType(*token, TOKEN_KW_RET)) {
        node = astNewNode(AST_RETURN);
        *token = (*token)->next;
        node->data.return_stmt.value = parseLogical(token);
        return node;
    }

    return parseAssign(token);
}

AstNode *parseAssign(Token **token) {
    if (isDeclarationStart(*token)) {
        AstNode *decl = astNewNode(AST_VAR_DECL);
        decl->data.var_decl.name = (*token)->value;
        *token = (*token)->next->next;
        decl->data.var_decl.declared_type = parseType(token);

        if (isTokenType(*token, TOKEN_ASSIGN)) {
            *token = (*token)->next;
            decl->data.var_decl.initializer = isTokenType(*token, TOKEN_LBRACE) ? parseArrayLiteral(token) : parseLogical(token);
        }

        return decl;
    }

    {
        AstNode *node = parseLogical(token);
        if (!isAtEnd(*token) && (isTokenType(*token, TOKEN_ASSIGN) || isTokenType(*token, TOKEN_PLUS_ASSIGN) || isTokenType(*token, TOKEN_MINUS_ASSIGN))) {
            AstNode *assign = astNewNode(AST_ASSIGN);
            assign->data.assign.op = assignOpFromToken((*token)->type);
            assign->data.assign.target = node;
            *token = (*token)->next;
            assign->data.assign.value = isTokenType(*token, TOKEN_LBRACE) ? parseArrayLiteral(token) : parseLogical(token);
            return assign;
        }
        return node;
    }
}

AstNode *parseLogical(Token **token) {
    AstNode *node = parseComparison(token);

    while (!isAtEnd(*token) && (isTokenType(*token, TOKEN_AND_AND) || isTokenType(*token, TOKEN_OR_OR))) {
        AstNode *binary = astNewNode(AST_BINARY);
        binary->data.binary.op = binaryOpFromToken((*token)->type);
        binary->data.binary.left = node;
        *token = (*token)->next;
        binary->data.binary.right = parseComparison(token);
        node = binary;
    }

    return node;
}

AstNode *parseComparison(Token **token) {
    AstNode *node = parseExpression(token);

    while (!isAtEnd(*token) && (isTokenType(*token, TOKEN_GT) || isTokenType(*token, TOKEN_LT) || isTokenType(*token, TOKEN_GT_EQ) || isTokenType(*token, TOKEN_LT_EQ) || isTokenType(*token, TOKEN_EQ_EQ) || isTokenType(*token, TOKEN_NOT_EQ))) {
        AstNode *binary = astNewNode(AST_BINARY);
        binary->data.binary.op = binaryOpFromToken((*token)->type);
        binary->data.binary.left = node;
        *token = (*token)->next;
        binary->data.binary.right = parseExpression(token);
        node = binary;
    }

    return node;
}

AstNode *parseExpression(Token **token) {
    AstNode *node = parseTerm(token);

    while (!isAtEnd(*token) && (isTokenType(*token, TOKEN_PLUS) || isTokenType(*token, TOKEN_MINUS))) {
        AstNode *binary = astNewNode(AST_BINARY);
        binary->data.binary.op = binaryOpFromToken((*token)->type);
        binary->data.binary.left = node;
        *token = (*token)->next;
        binary->data.binary.right = parseTerm(token);
        node = binary;
    }

    return node;
}

AstNode *parseTerm(Token **token) {
    AstNode *node = parseNegation(token);

    while (!isAtEnd(*token) && (isTokenType(*token, TOKEN_STAR) || isTokenType(*token, TOKEN_SLASH))) {
        AstNode *binary = astNewNode(AST_BINARY);
        binary->data.binary.op = binaryOpFromToken((*token)->type);
        binary->data.binary.left = node;
        *token = (*token)->next;
        binary->data.binary.right = parseNegation(token);
        node = binary;
    }

    return node;
}

AstNode *parseNegation(Token **token) {
    if (isTokenType(*token, TOKEN_BANG)) {
        AstNode *node = astNewNode(AST_UNARY);
        node->data.unary.op = AST_UNARY_NOT;
        *token = (*token)->next;
        node->data.unary.operand = parseNegation(token);
        return node;
    }

    return parseFactor(token);
}

AstNode *parseFactor(Token **token) {
    AstNode *node;

    if (isAtEnd(*token)) {
        syntaxError(*token, "unexpected end of input in expression");
        return NULL;
    }

    if (isTokenType(*token, TOKEN_NUMBER)) {
        node = astNewNode(AST_INT_LITERAL);
        node->data.int_literal.value = strtol((*token)->value, NULL, 10);
        *token = (*token)->next;
        return node;
    }
    if (isTokenType(*token, TOKEN_STRING)) {
        node = astNewNode(AST_STRING_LITERAL);
        node->data.string_literal.value = (*token)->value;
        *token = (*token)->next;
        return node;
    }
    if (isTokenType(*token, TOKEN_BOOL)) {
        node = astNewNode(AST_BOOL_LITERAL);
        node->data.bool_literal.value = strcmp((*token)->value, "true") == 0;
        *token = (*token)->next;
        return node;
    }
    if (isTokenType(*token, TOKEN_LBRACE)) {
        return parseArrayLiteral(token);
    }
    if (isTokenType(*token, TOKEN_IDENTIFIER)) {
        AstNode *base = astNewNode(AST_IDENTIFIER);
        base->data.identifier.name = (*token)->value;
        *token = (*token)->next;

        while (!isAtEnd(*token)) {
            if (isTokenType(*token, TOKEN_LPAREN)) {
                AstNode *call = astNewNode(AST_CALL);
                call->data.call.callee = base;
                *token = (*token)->next;
                while (!isTokenType(*token, TOKEN_RPAREN)) {
                    AstNode *arg = parseLogical(token);
                    astAppendNode(&call->data.call.args, &call->data.call.arg_count, arg);
                    if (isTokenType(*token, TOKEN_COMMA)) {
                        *token = (*token)->next;
                    }
                }
                expectToken(token, TOKEN_RPAREN, "expected ')' after call arguments");
                base = call;
                continue;
            }

            if (isTokenType(*token, TOKEN_LBRACKET)) {
                AstNode *access = astNewNode(AST_ARRAY_ACCESS);
                access->data.array_access.base = base;
                while (isTokenType(*token, TOKEN_LBRACKET)) {
                    AstNode *index;
                    *token = (*token)->next;
                    index = parseLogical(token);
                    astAppendNode(&access->data.array_access.indices, &access->data.array_access.index_count, index);
                    expectToken(token, TOKEN_RBRACKET, "expected ']' after array index");
                }
                base = access;
                continue;
            }

            break;
        }

        return base;
    }
    if (isTokenType(*token, TOKEN_LPAREN)) {
        *token = (*token)->next;
        node = parseLogical(token);
        expectToken(token, TOKEN_RPAREN, "expected ')' after grouped expression");
        return node;
    }

    syntaxError(*token, "unexpected token in expression");
    return NULL;
}

AstNode *parseType(Token **token) {
    AstNode *base;

    if (!isTypeToken(*token)) {
        syntaxError(*token, "expected type");
    }

    if (isTokenType(*token, TOKEN_TYPE_ARRAY)) {
        AstNode *arrayType = astNewNode(AST_TYPE_ARRAY);

        *token = (*token)->next;
        if (isTokenType(*token, TOKEN_LT)) {
            *token = (*token)->next;
            arrayType->data.type_array.element_type = parseType(token);
            expectToken(token, TOKEN_GT, "expected '>' after array element type");
        }

        return wrapArrayTypes(arrayType, token);
    }

    base = astNewNode(AST_TYPE_NAME);
    base->data.type_name.kind = typeKindFromToken(*token);
    base->data.type_name.name = (*token)->value;
    *token = (*token)->next;
    return wrapArrayTypes(base, token);
}

AstNode *parseArrayLiteral(Token **token) {
    AstNode *array = astNewNode(AST_ARRAY_LITERAL);

    expectToken(token, TOKEN_LBRACE, "expected '{' to start array literal");
    while (!isTokenType(*token, TOKEN_RBRACE)) {
        AstNode *element = isTokenType(*token, TOKEN_LBRACE) ? parseArrayLiteral(token) : parseLogical(token);
        astAppendNode(&array->data.array_literal.elements, &array->data.array_literal.count, element);
        if (isTokenType(*token, TOKEN_COMMA) || isTokenType(*token, TOKEN_SEMICOLON)) {
            *token = (*token)->next;
        }
    }
    expectToken(token, TOKEN_RBRACE, "expected '}' after array literal");
    return array;
}
