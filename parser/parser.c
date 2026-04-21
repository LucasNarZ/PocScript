#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static AstNode *parseBlock(Parser *parser);
static AstNode *parseStatement(Parser *parser);
static AstNode *parseReturn(Parser *parser);
static AstNode *parseAssign(Parser *parser);
static AstNode *parseLogical(Parser *parser);
static AstNode *parseComparison(Parser *parser);
static AstNode *parseExpression(Parser *parser);
static AstNode *parseTerm(Parser *parser);
static AstNode *parseNegation(Parser *parser);
static AstNode *parseFactor(Parser *parser);
static AstNode *parseType(Parser *parser);
static AstNode *parseArrayLiteral(Parser *parser);
static bool isAssignableTarget(const AstNode *node);

static AstNode *astNewNodeFromToken(AstNodeType type, const Token *token) {
    if (token == NULL) {
        return astNewNode(type);
    }

    return astNewNodeAt(type, token->line, token->column);
}

static AstNode *astNewNodeFromCurrent(Parser *parser, AstNodeType type) {
    return astNewNodeFromToken(type, parser->current);
}

static bool parserIs(const Parser *parser, TokenType type) {
    return parser->current != NULL && parser->current->type == type;
}

static char *copyTokenValue(const Token *token) {
    size_t length;
    char *copy;

    if (token == NULL || token->value == NULL) {
        return NULL;
    }

    length = strlen(token->value);
    copy = malloc(length + 1);
    if (copy == NULL) {
        fprintf(stderr, "Out of memory while copying token value\n");
        exit(1);
    }

    memcpy(copy, token->value, length + 1);
    return copy;
}

static bool parserAtEnd(const Parser *parser) {
    return parser->current == NULL || parser->current->type == TOKEN_EOF;
}

static void parserSyntaxError(const Token *token, const char *message) {
    if (token == NULL) {
        fprintf(stderr, "SyntaxError at end of input: %s\n", message);
        exit(1);
    }

    fprintf(stderr, "SyntaxError at line %d, column %d: %s\n", token->line, token->column, message);
    exit(1);
}

static Token *parserAdvance(Parser *parser) {
    Token *current = parser->current;

    if (parser->current != NULL) {
        parser->current = parser->current->next;
    }

    return current;
}

static void parserExpect(Parser *parser, TokenType type, const char *message) {
    if (!parserIs(parser, type)) {
        parserSyntaxError(parser->current, message);
    }

    parserAdvance(parser);
}

static bool isTypeToken(const Parser *parser) {
    return parserIs(parser, TOKEN_TYPE_ARRAY) || parserIs(parser, TOKEN_TYPE_INT) || parserIs(parser, TOKEN_TYPE_FLOAT) || parserIs(parser, TOKEN_TYPE_CHAR) || parserIs(parser, TOKEN_TYPE_BOOL) || parserIs(parser, TOKEN_IDENTIFIER);
}

static bool isDeclarationStart(const Parser *parser) {
    if (parser->current == NULL || parser->current->next == NULL) {
        return false;
    }

    return parser->current->type == TOKEN_IDENTIFIER && parser->current->next->type == TOKEN_DOUBLE_COLON;
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

static AstTypeKind typeKindFromToken(const Parser *parser) {
    switch (parser->current->type) {
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

static AstNode *wrapArrayTypes(AstNode *base, Parser *parser) {
    while (parserIs(parser, TOKEN_LBRACKET)) {
        AstNode *arrayType = astNewNodeFromCurrent(parser, AST_TYPE_ARRAY);
        AstNode *sizeExpr = NULL;

        parserAdvance(parser);
        if (!parserIs(parser, TOKEN_RBRACKET)) {
            sizeExpr = parseLogical(parser);
        }
        parserExpect(parser, TOKEN_RBRACKET, "expected ']' after array type size");
        arrayType->data.type_array.element_type = base;
        arrayType->data.type_array.size_expr = sizeExpr;
        base = arrayType;
    }

    return base;
}

static AstNode *parseIf(Parser *parser) {
    AstNode *node = astNewNodeFromCurrent(parser, AST_IF);

    parserAdvance(parser);
    parserExpect(parser, TOKEN_LPAREN, "expected '(' after if");
    node->data.if_stmt.condition = parseLogical(parser);
    parserExpect(parser, TOKEN_RPAREN, "expected ')' after if condition");
    node->data.if_stmt.then_branch = parseBlock(parser);

    if (parserIs(parser, TOKEN_KW_ELSE)) {
        parserAdvance(parser);
        if (parserIs(parser, TOKEN_KW_IF)) {
            node->data.if_stmt.else_branch = parseIf(parser);
        } else {
            node->data.if_stmt.else_branch = parseBlock(parser);
        }
    }

    return node;
}

static AstNode *parseWhile(Parser *parser) {
    AstNode *node = astNewNodeFromCurrent(parser, AST_WHILE);

    parserAdvance(parser);
    parserExpect(parser, TOKEN_LPAREN, "expected '(' after while");
    node->data.while_stmt.condition = parseLogical(parser);
    parserExpect(parser, TOKEN_RPAREN, "expected ')' after while condition");
    node->data.while_stmt.body = parseBlock(parser);
    return node;
}

static AstNode *parseFor(Parser *parser) {
    AstNode *node = astNewNodeFromCurrent(parser, AST_FOR);

    parserAdvance(parser);
    parserExpect(parser, TOKEN_LPAREN, "expected '(' after for");
    if (!parserIs(parser, TOKEN_SEMICOLON)) {
        node->data.for_stmt.init = parseAssign(parser);
    }
    parserExpect(parser, TOKEN_SEMICOLON, "expected ';' after for init");
    if (!parserIs(parser, TOKEN_SEMICOLON)) {
        node->data.for_stmt.condition = parseLogical(parser);
    }
    parserExpect(parser, TOKEN_SEMICOLON, "expected ';' after for condition");
    if (!parserIs(parser, TOKEN_RPAREN)) {
        node->data.for_stmt.update = parseAssign(parser);
    }
    parserExpect(parser, TOKEN_RPAREN, "expected ')' after for update");
    node->data.for_stmt.body = parseBlock(parser);
    return node;
}

static AstNode *parseParameter(Parser *parser) {
    AstNode *param = astNewNodeFromCurrent(parser, AST_PARAM);

    if (!parserIs(parser, TOKEN_IDENTIFIER)) {
        parserSyntaxError(parser->current, "expected parameter name");
    }

    param->data.param.name = copyTokenValue(parser->current);
    parserAdvance(parser);
    parserExpect(parser, TOKEN_DOUBLE_COLON, "expected '::' after parameter name");
    param->data.param.declared_type = parseType(parser);
    return param;
}

static AstNode *parseFunction(Parser *parser) {
    AstNode *node = astNewNodeFromCurrent(parser, AST_FUNC_DECL);

    parserAdvance(parser);
    if (!parserIs(parser, TOKEN_IDENTIFIER)) {
        parserSyntaxError(parser->current, "expected function name after func");
    }

    node->data.func_decl.name = copyTokenValue(parser->current);
    parserAdvance(parser);
    parserExpect(parser, TOKEN_LPAREN, "expected '(' after function name");

    if (!parserIs(parser, TOKEN_RPAREN)) {
        AstNode *param = parseParameter(parser);
        astAppendNode(&node->data.func_decl.params, &node->data.func_decl.param_count, param);

        while (parserIs(parser, TOKEN_COMMA)) {
            parserAdvance(parser);
            param = parseParameter(parser);
            astAppendNode(&node->data.func_decl.params, &node->data.func_decl.param_count, param);
        }
    }

    parserExpect(parser, TOKEN_RPAREN, "expected ')' after function parameters");
    node->data.func_decl.body = parseBlock(parser);
    return node;
}

void parserInit(Parser *parser, Token *tokens) {
    parser->tokens = tokens;
    parser->current = tokens;
}

AstNode *parserParseProgram(Parser *parser) {
    AstNode *program = astNewNodeFromCurrent(parser, AST_PROGRAM);

    while (!parserAtEnd(parser)) {
        AstNode *item = parseStatement(parser);
        if (item != NULL) {
            astAppendNode(&program->data.program.items, &program->data.program.count, item);
        }
    }

    return program;
}

static AstNode *parseBlock(Parser *parser) {
    AstNode *block = astNewNodeFromCurrent(parser, AST_BLOCK);

    parserExpect(parser, TOKEN_LBRACE, "expected '{' to start block");

    while (!parserAtEnd(parser) && !parserIs(parser, TOKEN_RBRACE)) {
        AstNode *item = parseStatement(parser);
        if (item != NULL) {
            astAppendNode(&block->data.block.items, &block->data.block.count, item);
        }
    }

    if (parserIs(parser, TOKEN_RBRACE)) {
        parserAdvance(parser);
    } else {
        parserSyntaxError(parser->current, "expected '}' to close block");
    }

    return block;
}

static AstNode *parseStatement(Parser *parser) {
    AstNode *node;

    if (parserAtEnd(parser)) {
        return NULL;
    }

    if (parserIs(parser, TOKEN_KW_IF)) {
        return parseIf(parser);
    }
    if (parserIs(parser, TOKEN_KW_WHILE)) {
        return parseWhile(parser);
    }
    if (parserIs(parser, TOKEN_KW_FOR)) {
        return parseFor(parser);
    }
    if (parserIs(parser, TOKEN_KW_FUNC)) {
        return parseFunction(parser);
    }

    node = parseReturn(parser);
    parserExpect(parser, TOKEN_SEMICOLON, "expected ';' after statement");

    if (node->type == AST_ASSIGN || node->type == AST_VAR_DECL || node->type == AST_RETURN) {
        return node;
    }

    {
        AstNode *exprStmt = astNewNode(AST_EXPR_STMT);
        exprStmt->line = node->line;
        exprStmt->column = node->column;
        exprStmt->data.expr_stmt.expression = node;
        return exprStmt;
    }
}

static AstNode *parseReturn(Parser *parser) {
    AstNode *node;

    if (parserIs(parser, TOKEN_KW_RET)) {
        node = astNewNodeFromCurrent(parser, AST_RETURN);
        parserAdvance(parser);
        node->data.return_stmt.value = parseLogical(parser);
        return node;
    }

    return parseAssign(parser);
}

static AstNode *parseAssign(Parser *parser) {
    if (isDeclarationStart(parser)) {
        AstNode *decl = astNewNodeFromCurrent(parser, AST_VAR_DECL);
        decl->data.var_decl.name = copyTokenValue(parser->current);
        parserAdvance(parser);
        parserExpect(parser, TOKEN_DOUBLE_COLON, "expected '::' after declaration name");
        decl->data.var_decl.declared_type = parseType(parser);

        if (parserIs(parser, TOKEN_ASSIGN)) {
            parserAdvance(parser);
            decl->data.var_decl.initializer = parseLogical(parser);
        }

        return decl;
    }

    {
        AstNode *node = parseLogical(parser);
        if (!parserAtEnd(parser) && (parserIs(parser, TOKEN_ASSIGN) || parserIs(parser, TOKEN_PLUS_ASSIGN) || parserIs(parser, TOKEN_MINUS_ASSIGN))) {
            if (!isAssignableTarget(node)) {
                parserSyntaxError(parser->current, "invalid assignment target");
            }

            AstNode *assign = astNewNodeFromCurrent(parser, AST_ASSIGN);
            assign->data.assign.op = assignOpFromToken(parser->current->type);
            assign->data.assign.target = node;
            parserAdvance(parser);
            assign->data.assign.value = parseLogical(parser);
            return assign;
        }
        return node;
    }
}

static AstNode *parseLogical(Parser *parser) {
    AstNode *node = parseComparison(parser);

    while (!parserAtEnd(parser) && (parserIs(parser, TOKEN_AND_AND) || parserIs(parser, TOKEN_OR_OR))) {
        AstNode *binary = astNewNodeFromCurrent(parser, AST_BINARY);
        binary->data.binary.op = binaryOpFromToken(parser->current->type);
        binary->data.binary.left = node;
        parserAdvance(parser);
        binary->data.binary.right = parseComparison(parser);
        node = binary;
    }

    return node;
}

static AstNode *parseComparison(Parser *parser) {
    AstNode *node = parseExpression(parser);

    while (!parserAtEnd(parser) && (parserIs(parser, TOKEN_GT) || parserIs(parser, TOKEN_LT) || parserIs(parser, TOKEN_GT_EQ) || parserIs(parser, TOKEN_LT_EQ) || parserIs(parser, TOKEN_EQ_EQ) || parserIs(parser, TOKEN_NOT_EQ))) {
        AstNode *binary = astNewNodeFromCurrent(parser, AST_BINARY);
        binary->data.binary.op = binaryOpFromToken(parser->current->type);
        binary->data.binary.left = node;
        parserAdvance(parser);
        binary->data.binary.right = parseExpression(parser);
        node = binary;
    }

    return node;
}

static AstNode *parseExpression(Parser *parser) {
    AstNode *node = parseTerm(parser);

    while (!parserAtEnd(parser) && (parserIs(parser, TOKEN_PLUS) || parserIs(parser, TOKEN_MINUS))) {
        AstNode *binary = astNewNodeFromCurrent(parser, AST_BINARY);
        binary->data.binary.op = binaryOpFromToken(parser->current->type);
        binary->data.binary.left = node;
        parserAdvance(parser);
        binary->data.binary.right = parseTerm(parser);
        node = binary;
    }

    return node;
}

static AstNode *parseTerm(Parser *parser) {
    AstNode *node = parseNegation(parser);

    while (!parserAtEnd(parser) && (parserIs(parser, TOKEN_STAR) || parserIs(parser, TOKEN_SLASH))) {
        AstNode *binary = astNewNodeFromCurrent(parser, AST_BINARY);
        binary->data.binary.op = binaryOpFromToken(parser->current->type);
        binary->data.binary.left = node;
        parserAdvance(parser);
        binary->data.binary.right = parseNegation(parser);
        node = binary;
    }

    return node;
}

static AstNode *parseNegation(Parser *parser) {
    if (parserIs(parser, TOKEN_BANG)) {
        AstNode *node = astNewNodeFromCurrent(parser, AST_UNARY);
        node->data.unary.op = AST_UNARY_NOT;
        parserAdvance(parser);
        node->data.unary.operand = parseNegation(parser);
        return node;
    }

    return parseFactor(parser);
}

static AstNode *parseFactor(Parser *parser) {
    AstNode *node;

    if (parserAtEnd(parser)) {
        parserSyntaxError(parser->current, "unexpected end of input in expression");
        return NULL;
    }

    if (parserIs(parser, TOKEN_NUMBER)) {
        if (strchr(parser->current->value, '.') != NULL) {
            node = astNewNodeFromCurrent(parser, AST_FLOAT_LITERAL);
            node->data.float_literal.value = strtod(parser->current->value, NULL);
        } else {
            node = astNewNodeFromCurrent(parser, AST_INT_LITERAL);
            node->data.int_literal.value = strtol(parser->current->value, NULL, 10);
        }
        parserAdvance(parser);
        return node;
    }
    if (parserIs(parser, TOKEN_STRING)) {
        node = astNewNodeFromCurrent(parser, AST_STRING_LITERAL);
        node->data.string_literal.value = copyTokenValue(parser->current);
        parserAdvance(parser);
        return node;
    }
    if (parserIs(parser, TOKEN_BOOL)) {
        node = astNewNodeFromCurrent(parser, AST_BOOL_LITERAL);
        node->data.bool_literal.value = strcmp(parser->current->value, "true") == 0;
        parserAdvance(parser);
        return node;
    }
    if (parserIs(parser, TOKEN_LBRACE)) {
        return parseArrayLiteral(parser);
    }
    if (parserIs(parser, TOKEN_IDENTIFIER)) {
        AstNode *base = astNewNodeFromCurrent(parser, AST_IDENTIFIER);
        base->data.identifier.name = copyTokenValue(parser->current);
        parserAdvance(parser);

        while (!parserAtEnd(parser)) {
            if (parserIs(parser, TOKEN_LPAREN)) {
                AstNode *call = astNewNodeAt(AST_CALL, base->line, base->column);
                call->data.call.callee = base;
                parserAdvance(parser);
                if (!parserIs(parser, TOKEN_RPAREN)) {
                    AstNode *arg = parseLogical(parser);
                    astAppendNode(&call->data.call.args, &call->data.call.arg_count, arg);

                    while (parserIs(parser, TOKEN_COMMA)) {
                        parserAdvance(parser);
                        arg = parseLogical(parser);
                        astAppendNode(&call->data.call.args, &call->data.call.arg_count, arg);
                    }
                }
                parserExpect(parser, TOKEN_RPAREN, "expected ')' after call arguments");
                base = call;
                continue;
            }

            if (parserIs(parser, TOKEN_LBRACKET)) {
                AstNode *access = astNewNodeAt(AST_ARRAY_ACCESS, base->line, base->column);
                AstNode *index;

                access->data.array_access.base = base;
                parserAdvance(parser);
                index = parseLogical(parser);
                astAppendNode(&access->data.array_access.indices, &access->data.array_access.index_count, index);
                parserExpect(parser, TOKEN_RBRACKET, "expected ']' after array index");
                base = access;
                continue;
            }

            break;
        }

        return base;
    }
    if (parserIs(parser, TOKEN_LPAREN)) {
        parserAdvance(parser);
        node = parseLogical(parser);
        parserExpect(parser, TOKEN_RPAREN, "expected ')' after grouped expression");
        return node;
    }

    parserSyntaxError(parser->current, "unexpected token in expression");
    return NULL;
}

static bool isAssignableTarget(const AstNode *node) {
    if (node == NULL) {
        return false;
    }

    if (node->type == AST_IDENTIFIER) {
        return true;
    }

    if (node->type == AST_ARRAY_ACCESS) {
        return isAssignableTarget(node->data.array_access.base);
    }

    return false;
}

static AstNode *parseType(Parser *parser) {
    AstNode *base;

    if (!isTypeToken(parser)) {
        parserSyntaxError(parser->current, "expected type");
    }

    if (parserIs(parser, TOKEN_TYPE_ARRAY)) {
        AstNode *arrayType = astNewNodeFromCurrent(parser, AST_TYPE_ARRAY);

        parserAdvance(parser);
        if (parserIs(parser, TOKEN_LT)) {
            parserAdvance(parser);
            arrayType->data.type_array.element_type = parseType(parser);
            parserExpect(parser, TOKEN_GT, "expected '>' after array element type");
        }

        return wrapArrayTypes(arrayType, parser);
    }

    base = astNewNodeFromCurrent(parser, AST_TYPE_NAME);
    base->data.type_name.kind = typeKindFromToken(parser);
    base->data.type_name.name = copyTokenValue(parser->current);
    parserAdvance(parser);
    return wrapArrayTypes(base, parser);
}

static AstNode *parseArrayLiteral(Parser *parser) {
    AstNode *array = astNewNodeFromCurrent(parser, AST_ARRAY_LITERAL);

    parserExpect(parser, TOKEN_LBRACE, "expected '{' to start array literal");

    if (!parserIs(parser, TOKEN_RBRACE)) {
        AstNode *element = parseLogical(parser);
        astAppendNode(&array->data.array_literal.elements, &array->data.array_literal.count, element);

        while (parserIs(parser, TOKEN_COMMA) || parserIs(parser, TOKEN_SEMICOLON)) {
            parserAdvance(parser);
            if (parserIs(parser, TOKEN_RBRACE) || parserAtEnd(parser)) {
                break;
            }

            element = parseLogical(parser);
            astAppendNode(&array->data.array_literal.elements, &array->data.array_literal.count, element);
        }
    }

    parserExpect(parser, TOKEN_RBRACE, "expected '}' after array literal");
    return array;
}
