#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct AstNode AstNode;

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_FUNC_DECL,
    AST_PARAM,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_EXPR_STMT,
    AST_ASSIGN,
    AST_BINARY,
    AST_UNARY,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_ARRAY_LITERAL,
    AST_IDENTIFIER,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_BOOL_LITERAL,
    AST_TYPE_NAME,
    AST_TYPE_ARRAY
} AstNodeType;

typedef enum {
    AST_TYPE_INT_KIND,
    AST_TYPE_FLOAT_KIND,
    AST_TYPE_CHAR_KIND,
    AST_TYPE_BOOL_KIND,
    AST_TYPE_ARRAY_KIND,
    AST_TYPE_CUSTOM_KIND
} AstTypeKind;

typedef enum {
    AST_BINARY_ADD,
    AST_BINARY_SUB,
    AST_BINARY_MUL,
    AST_BINARY_DIV,
    AST_BINARY_GT,
    AST_BINARY_LT,
    AST_BINARY_GTE,
    AST_BINARY_LTE,
    AST_BINARY_EQ,
    AST_BINARY_NEQ,
    AST_BINARY_AND,
    AST_BINARY_OR
} AstBinaryOp;

typedef enum {
    AST_UNARY_NEGATE,
    AST_UNARY_NOT
} AstUnaryOp;

typedef enum {
    AST_ASSIGN_SET,
    AST_ASSIGN_ADD,
    AST_ASSIGN_SUB
} AstAssignOp;

typedef struct {
    AstNode **items;
    size_t count;
} AstNodeList;

typedef struct {
    AstNode **elements;
    size_t count;
} AstArrayElements;

typedef struct {
    AstNode *init;
    AstNode *condition;
    AstNode *update;
    AstNode *body;
} AstForData;

typedef struct {
    AstNode *condition;
    AstNode *then_branch;
    AstNode *else_branch;
} AstIfData;

typedef struct {
    AstNode *condition;
    AstNode *body;
} AstWhileData;

typedef struct {
    char *name;
    AstNode *declared_type;
    AstNode *initializer;
} AstVarDeclData;

typedef struct {
    char *name;
    AstNode **params;
    size_t param_count;
    AstNode *body;
    AstNode *return_type;
} AstFuncDeclData;

typedef struct {
    char *name;
    AstNode *declared_type;
} AstParamData;

typedef struct {
    AstAssignOp op;
    AstNode *target;
    AstNode *value;
} AstAssignData;

typedef struct {
    AstBinaryOp op;
    AstNode *left;
    AstNode *right;
} AstBinaryData;

typedef struct {
    AstUnaryOp op;
    AstNode *operand;
} AstUnaryData;

typedef struct {
    AstNode *callee;
    AstNode **args;
    size_t arg_count;
} AstCallData;

typedef struct {
    AstNode *base;
    AstNode **indices;
    size_t index_count;
} AstArrayAccessData;

typedef struct {
    AstTypeKind kind;
    char *name;
} AstTypeNameData;

typedef struct {
    AstNode *element_type;
    AstNode *size_expr;
} AstTypeArrayData;

struct AstNode {
    AstNodeType type;
    union {
        AstNodeList program;
        AstNodeList block;
        AstVarDeclData var_decl;
        AstFuncDeclData func_decl;
        AstParamData param;
        AstIfData if_stmt;
        AstWhileData while_stmt;
        AstForData for_stmt;
        struct {
            AstNode *value;
        } return_stmt;
        struct {
            AstNode *expression;
        } expr_stmt;
        AstAssignData assign;
        AstBinaryData binary;
        AstUnaryData unary;
        AstCallData call;
        AstArrayAccessData array_access;
        AstArrayElements array_literal;
        struct {
            char *name;
        } identifier;
        struct {
            long value;
        } int_literal;
        struct {
            double value;
        } float_literal;
        struct {
            char *value;
        } string_literal;
        struct {
            bool value;
        } bool_literal;
        AstTypeNameData type_name;
        AstTypeArrayData type_array;
    } data;
};

AstNode *astNewNode(AstNodeType type);
void astAppendNode(AstNode ***items, size_t *count, AstNode *item);
char *astToString(AstNode *root);
void astPrintPretty(AstNode *root);
void astFree(AstNode *root);

#endif
