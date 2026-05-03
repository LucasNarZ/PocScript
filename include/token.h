#ifndef TOKEN_H
#define TOKEN_H

#include "constants.h"

typedef enum {
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_COMMENT,
    TOKEN_TYPE_ARRAY,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_CHAR,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_VOID,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_FOR,
    TOKEN_KW_WHILE,
    TOKEN_KW_FUNC,
    TOKEN_KW_EXTERN,
    TOKEN_KW_RET,
    TOKEN_KW_BREAK,
    TOKEN_KW_CONTINUE,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
    TOKEN_EQ_EQ,
    TOKEN_GT_EQ,
    TOKEN_LT_EQ,
    TOKEN_NOT_EQ,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_BANG,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_ARROW,
    TOKEN_ASSIGN,
    TOKEN_DOUBLE_COLON,
    TOKEN_IDENTIFIER,
    TOKEN_OR_OR,
    TOKEN_AND_AND,
    TOKEN_AMPERSAND,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_EOF,
    TOKEN_WHITESPACE
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    int line;
    int column;
    struct Token *next;
    struct Token *anterior;
} Token;

typedef struct {
    TokenType type;
    char *pattern;
} TokenDefinition;

#endif
