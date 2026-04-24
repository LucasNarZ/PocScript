#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AstNode *astNewNode(AstNodeType type) {
    return astNewNodeAt(type, 0, 0);
}

AstNode *astNewNodeAt(AstNodeType type, int line, int column) {
    AstNode *node = calloc(1, sizeof(AstNode));

    if (node == NULL) {
        return NULL;
    }

    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

void astAppendNode(AstNode ***items, size_t *count, AstNode *item) {
    AstNode **nextItems = realloc(*items, (*count + 1) * sizeof(AstNode *));

    if (nextItems == NULL) {
        return;
    }

    *items = nextItems;
    (*items)[*count] = item;
    (*count)++;
}

static const char *binaryOpName(AstBinaryOp op) {
    switch (op) {
        case AST_BINARY_ADD: return "+";
        case AST_BINARY_SUB: return "-";
        case AST_BINARY_MUL: return "*";
        case AST_BINARY_DIV: return "/";
        case AST_BINARY_GT: return ">";
        case AST_BINARY_LT: return "<";
        case AST_BINARY_GTE: return ">=";
        case AST_BINARY_LTE: return "<=";
        case AST_BINARY_EQ: return "==";
        case AST_BINARY_NEQ: return "!=";
        case AST_BINARY_AND: return "&&";
        case AST_BINARY_OR: return "||";
    }

    return "?";
}

static const char *assignOpName(AstAssignOp op) {
    switch (op) {
        case AST_ASSIGN_SET: return "=";
        case AST_ASSIGN_ADD: return "+=";
        case AST_ASSIGN_SUB: return "-=";
    }

    return "=";
}

static void appendText(char **buffer, size_t *size, const char *text) {
    size_t textLength = strlen(text);
    char *nextBuffer = realloc(*buffer, *size + textLength);

    if (nextBuffer == NULL) {
        free(*buffer);
        *buffer = NULL;
        return;
    }

    *buffer = nextBuffer;
    memcpy(*buffer + *size - 1, text, textLength + 1);
    *size += textLength;
}

static void appendFormat(char **buffer, size_t *size, const char *text) {
    if (*buffer == NULL) {
        return;
    }
    appendText(buffer, size, text);
}

static void writeIndent(char **buffer, size_t *size, int depth, int isLast[]) {
    int i;
    for (i = 0; i < depth - 1; i++) {
        appendFormat(buffer, size, isLast[i] ? "   " : "  |");
    }
    if (depth > 0) {
        appendFormat(buffer, size, "  ├─");
    }
}

static void writeNode(char **buffer, size_t *size, AstNode *node, int depth, int isLast[]);

static void freeNodeList(AstNode **items, size_t count) {
    size_t i;

    for (i = 0; i < count; i++) {
        astFree(items[i]);
    }

    free(items);
}

static void writeList(char **buffer, size_t *size, AstNode **items, size_t count, int depth, int isLast[]) {
    size_t i;
    for (i = 0; i < count; i++) {
        isLast[depth] = (i == count - 1);
        writeNode(buffer, size, items[i], depth + 1, isLast);
    }
}

static void writeLabel(char **buffer, size_t *size, AstNode *node) {
    char temp[64];
    switch (node->type) {
        case AST_PROGRAM: appendFormat(buffer, size, " (PROGRAM)\n"); break;
        case AST_BLOCK: appendFormat(buffer, size, " (BLOCK)\n"); break;
        case AST_VAR_DECL: appendFormat(buffer, size, " (VAR_DECL "); appendFormat(buffer, size, node->data.var_decl.name); appendFormat(buffer, size, ")\n"); break;
        case AST_FUNC_DECL: appendFormat(buffer, size, " (FUNC_DECL "); appendFormat(buffer, size, node->data.func_decl.name); appendFormat(buffer, size, ")\n"); break;
        case AST_PARAM: appendFormat(buffer, size, " (PARAM "); appendFormat(buffer, size, node->data.param.name); appendFormat(buffer, size, ")\n"); break;
        case AST_IF: appendFormat(buffer, size, " (IF)\n"); break;
        case AST_WHILE: appendFormat(buffer, size, " (WHILE)\n"); break;
        case AST_FOR: appendFormat(buffer, size, " (FOR)\n"); break;
        case AST_RETURN: appendFormat(buffer, size, " (RETURN)\n"); break;
        case AST_BREAK: appendFormat(buffer, size, " (BREAK)\n"); break;
        case AST_CONTINUE: appendFormat(buffer, size, " (CONTINUE)\n"); break;
        case AST_EXPR_STMT: appendFormat(buffer, size, " (EXPR_STMT)\n"); break;
        case AST_ASSIGN: appendFormat(buffer, size, " ("); appendFormat(buffer, size, assignOpName(node->data.assign.op)); appendFormat(buffer, size, ")\n"); break;
        case AST_BINARY: appendFormat(buffer, size, " ("); appendFormat(buffer, size, binaryOpName(node->data.binary.op)); appendFormat(buffer, size, ")\n"); break;
        case AST_UNARY: appendFormat(buffer, size, " (UNARY)\n"); break;
        case AST_CALL: appendFormat(buffer, size, " (CALL)\n"); break;
        case AST_ARRAY_ACCESS: appendFormat(buffer, size, " (ARRAY_ACCESS)\n"); break;
        case AST_ARRAY_LITERAL: appendFormat(buffer, size, " (ARRAY_LITERAL)\n"); break;
        case AST_IDENTIFIER: appendFormat(buffer, size, " ("); appendFormat(buffer, size, node->data.identifier.name); appendFormat(buffer, size, ")\n"); break;
        case AST_INT_LITERAL:
            snprintf(temp, sizeof(temp), " (%ld)\n", node->data.int_literal.value);
            appendFormat(buffer, size, temp);
            break;
        case AST_FLOAT_LITERAL:
            snprintf(temp, sizeof(temp), " (%g)\n", node->data.float_literal.value);
            appendFormat(buffer, size, temp);
            break;
        case AST_STRING_LITERAL: appendFormat(buffer, size, " ("); appendFormat(buffer, size, node->data.string_literal.value); appendFormat(buffer, size, ")\n"); break;
        case AST_BOOL_LITERAL: appendFormat(buffer, size, node->data.bool_literal.value ? " (true)\n" : " (false)\n"); break;
        case AST_TYPE_NAME: appendFormat(buffer, size, " (TYPE "); appendFormat(buffer, size, node->data.type_name.name); appendFormat(buffer, size, ")\n"); break;
        case AST_TYPE_ARRAY: appendFormat(buffer, size, " (TYPE_ARRAY)\n"); break;
    }
}

static void writeNode(char **buffer, size_t *size, AstNode *node, int depth, int isLast[]) {
    if (node == NULL || *buffer == NULL) {
        return;
    }

    writeIndent(buffer, size, depth, isLast);
    writeLabel(buffer, size, node);

    switch (node->type) {
        case AST_PROGRAM:
            writeList(buffer, size, node->data.program.items, node->data.program.count, depth, isLast);
            break;
        case AST_BLOCK:
            writeList(buffer, size, node->data.block.items, node->data.block.count, depth, isLast);
            break;
        case AST_VAR_DECL:
            if (node->data.var_decl.declared_type) writeNode(buffer, size, node->data.var_decl.declared_type, depth + 1, isLast);
            if (node->data.var_decl.initializer) writeNode(buffer, size, node->data.var_decl.initializer, depth + 1, isLast);
            break;
        case AST_FUNC_DECL:
            if (node->data.func_decl.return_type) writeNode(buffer, size, node->data.func_decl.return_type, depth + 1, isLast);
            writeList(buffer, size, node->data.func_decl.params, node->data.func_decl.param_count, depth, isLast);
            if (node->data.func_decl.body) writeNode(buffer, size, node->data.func_decl.body, depth + 1, isLast);
            break;
        case AST_PARAM:
            if (node->data.param.declared_type) writeNode(buffer, size, node->data.param.declared_type, depth + 1, isLast);
            break;
        case AST_IF:
            writeNode(buffer, size, node->data.if_stmt.condition, depth + 1, isLast);
            writeNode(buffer, size, node->data.if_stmt.then_branch, depth + 1, isLast);
            if (node->data.if_stmt.else_branch) writeNode(buffer, size, node->data.if_stmt.else_branch, depth + 1, isLast);
            break;
        case AST_WHILE:
            writeNode(buffer, size, node->data.while_stmt.condition, depth + 1, isLast);
            writeNode(buffer, size, node->data.while_stmt.body, depth + 1, isLast);
            break;
        case AST_FOR:
            if (node->data.for_stmt.init) writeNode(buffer, size, node->data.for_stmt.init, depth + 1, isLast);
            if (node->data.for_stmt.condition) writeNode(buffer, size, node->data.for_stmt.condition, depth + 1, isLast);
            if (node->data.for_stmt.update) writeNode(buffer, size, node->data.for_stmt.update, depth + 1, isLast);
            if (node->data.for_stmt.body) writeNode(buffer, size, node->data.for_stmt.body, depth + 1, isLast);
            break;
        case AST_RETURN:
            if (node->data.return_stmt.value) writeNode(buffer, size, node->data.return_stmt.value, depth + 1, isLast);
            break;
        case AST_BREAK:
        case AST_CONTINUE:
            break;
        case AST_EXPR_STMT:
            if (node->data.expr_stmt.expression) writeNode(buffer, size, node->data.expr_stmt.expression, depth + 1, isLast);
            break;
        case AST_ASSIGN:
            writeNode(buffer, size, node->data.assign.target, depth + 1, isLast);
            writeNode(buffer, size, node->data.assign.value, depth + 1, isLast);
            break;
        case AST_BINARY:
            writeNode(buffer, size, node->data.binary.left, depth + 1, isLast);
            writeNode(buffer, size, node->data.binary.right, depth + 1, isLast);
            break;
        case AST_UNARY:
            writeNode(buffer, size, node->data.unary.operand, depth + 1, isLast);
            break;
        case AST_CALL:
            writeNode(buffer, size, node->data.call.callee, depth + 1, isLast);
            writeList(buffer, size, node->data.call.args, node->data.call.arg_count, depth, isLast);
            break;
        case AST_ARRAY_ACCESS:
            writeNode(buffer, size, node->data.array_access.base, depth + 1, isLast);
            writeList(buffer, size, node->data.array_access.indices, node->data.array_access.index_count, depth, isLast);
            break;
        case AST_ARRAY_LITERAL:
            writeList(buffer, size, node->data.array_literal.elements, node->data.array_literal.count, depth, isLast);
            break;
        case AST_TYPE_ARRAY:
            if (node->data.type_array.element_type) writeNode(buffer, size, node->data.type_array.element_type, depth + 1, isLast);
            if (node->data.type_array.size_expr) writeNode(buffer, size, node->data.type_array.size_expr, depth + 1, isLast);
            break;
        default:
            break;
    }
}

char *astToString(AstNode *root) {
    char *buffer = malloc(1);
    size_t size = 1;
    int isLast[128] = {0};

    if (buffer == NULL) {
        return NULL;
    }

    buffer[0] = '\0';
    writeNode(&buffer, &size, root, 0, isLast);
    return buffer;
}

void astPrintPretty(AstNode *root) {
    char *text = astToString(root);

    if (text != NULL) {
        fputs(text, stdout);
        free(text);
    }
}

void astFree(AstNode *root) {
    if (root == NULL) {
        return;
    }

    switch (root->type) {
        case AST_PROGRAM:
            freeNodeList(root->data.program.items, root->data.program.count);
            break;
        case AST_BLOCK:
            freeNodeList(root->data.block.items, root->data.block.count);
            break;
        case AST_VAR_DECL:
            free(root->data.var_decl.name);
            astFree(root->data.var_decl.declared_type);
            astFree(root->data.var_decl.initializer);
            break;
        case AST_FUNC_DECL:
            free(root->data.func_decl.name);
            freeNodeList(root->data.func_decl.params, root->data.func_decl.param_count);
            astFree(root->data.func_decl.body);
            astFree(root->data.func_decl.return_type);
            break;
        case AST_PARAM:
            free(root->data.param.name);
            astFree(root->data.param.declared_type);
            break;
        case AST_IF:
            astFree(root->data.if_stmt.condition);
            astFree(root->data.if_stmt.then_branch);
            astFree(root->data.if_stmt.else_branch);
            break;
        case AST_WHILE:
            astFree(root->data.while_stmt.condition);
            astFree(root->data.while_stmt.body);
            break;
        case AST_FOR:
            astFree(root->data.for_stmt.init);
            astFree(root->data.for_stmt.condition);
            astFree(root->data.for_stmt.update);
            astFree(root->data.for_stmt.body);
            break;
        case AST_RETURN:
            astFree(root->data.return_stmt.value);
            break;
        case AST_BREAK:
        case AST_CONTINUE:
            break;
        case AST_EXPR_STMT:
            astFree(root->data.expr_stmt.expression);
            break;
        case AST_ASSIGN:
            astFree(root->data.assign.target);
            astFree(root->data.assign.value);
            break;
        case AST_BINARY:
            astFree(root->data.binary.left);
            astFree(root->data.binary.right);
            break;
        case AST_UNARY:
            astFree(root->data.unary.operand);
            break;
        case AST_CALL:
            astFree(root->data.call.callee);
            freeNodeList(root->data.call.args, root->data.call.arg_count);
            break;
        case AST_ARRAY_ACCESS:
            astFree(root->data.array_access.base);
            freeNodeList(root->data.array_access.indices, root->data.array_access.index_count);
            break;
        case AST_ARRAY_LITERAL:
            freeNodeList(root->data.array_literal.elements, root->data.array_literal.count);
            break;
        case AST_IDENTIFIER:
            free(root->data.identifier.name);
            break;
        case AST_STRING_LITERAL:
            free(root->data.string_literal.value);
            break;
        case AST_TYPE_NAME:
            free(root->data.type_name.name);
            break;
        case AST_TYPE_ARRAY:
            astFree(root->data.type_array.element_type);
            astFree(root->data.type_array.size_expr);
            break;
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_BOOL_LITERAL:
            break;
    }

    free(root);
}
