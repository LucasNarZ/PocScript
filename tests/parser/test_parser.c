#include "../helpers/test_helpers.h"
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../lexer/lexer.h"
#include "../../parser/parser.h"

static Token *makeToken(TokenType type, char *value) {
    Token *token = malloc(sizeof(Token));

    token->type = type;
    token->value = value;
    token->next = NULL;
    token->anterior = NULL;
    return token;
}

static void appendToken(Token **head, Token *token) {
    Token *current;

    if (*head == NULL) {
        *head = token;
        return;
    }

    current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = token;
    token->anterior = current;
}

static void freeTokenList(Token *head) {
    Token *current = head;

    while (current != NULL) {
        Token *next = current->next;
        free(current);
        current = next;
    }
}

static AstNode *parseRoot(Token *tokens) {
    Parser parser;

    parserInit(&parser, tokens);
    return parserParseProgram(&parser);
}

static void assertParseError(const char *input, const char *expectedMessage) {
    char pathTemplate[] = "/tmp/pocscript-parser-error-XXXXXX";
    char buffer[512];
    pid_t pid;
    int fd;
    int status;
    ssize_t bytesRead;

    fd = mkstemp(pathTemplate);
    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        return;
    }

    fflush(stdout);
    pid = fork();
    EXPECT_TRUE(pid >= 0);
    if (pid < 0) {
        close(fd);
        unlink(pathTemplate);
        return;
    }

    if (pid == 0) {
        Token *tokens = tokenizeString(input);
        Parser parser;

        dup2(fd, STDERR_FILENO);
        close(fd);
        parserInit(&parser, tokens);
        parserParseProgram(&parser);
        freeTokens(tokens);
        _exit(0);
    }

    close(fd);
    EXPECT_TRUE(waitpid(pid, &status, 0) == pid);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_TRUE(WEXITSTATUS(status) == 1);

    fd = open(pathTemplate, O_RDONLY);
    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        unlink(pathTemplate);
        return;
    }

    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    EXPECT_TRUE(bytesRead >= 0);
    if (bytesRead >= 0) {
        buffer[bytesRead] = '\0';
        if (strstr(buffer, expectedMessage) == NULL) {
            fprintf(stderr, "Expected parser error containing '%s', got '%s'\n", expectedMessage, buffer);
        }
        EXPECT_TRUE(strstr(buffer, expectedMessage) != NULL);
    }

    close(fd);
    unlink(pathTemplate);
}

static void assertParseErrorAt(const char *input, const char *expectedMessage, int expectedLine, int expectedColumn) {
    char pathTemplate[] = "/tmp/pocscript-parser-error-at-XXXXXX";
    char buffer[512];
    char expectedPrefix[128];
    pid_t pid;
    int fd;
    int status;
    ssize_t bytesRead;

    fd = mkstemp(pathTemplate);
    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        return;
    }

    fflush(stdout);
    pid = fork();
    EXPECT_TRUE(pid >= 0);
    if (pid < 0) {
        close(fd);
        unlink(pathTemplate);
        return;
    }

    if (pid == 0) {
        Token *tokens = tokenizeString(input);
        Parser parser;

        dup2(fd, STDERR_FILENO);
        close(fd);
        parserInit(&parser, tokens);
        parserParseProgram(&parser);
        freeTokens(tokens);
        _exit(0);
    }

    close(fd);
    EXPECT_TRUE(waitpid(pid, &status, 0) == pid);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_TRUE(WEXITSTATUS(status) == 1);

    fd = open(pathTemplate, O_RDONLY);
    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        unlink(pathTemplate);
        return;
    }

    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    EXPECT_TRUE(bytesRead >= 0);
    if (bytesRead >= 0) {
        buffer[bytesRead] = '\0';
        snprintf(expectedPrefix, sizeof(expectedPrefix), "SyntaxError at line %d, column %d:", expectedLine, expectedColumn);
        if (strstr(buffer, expectedPrefix) == NULL) {
            fprintf(stderr, "Expected parser error position '%s', got '%s'\n", expectedPrefix, buffer);
        }
        EXPECT_TRUE(strstr(buffer, expectedPrefix) != NULL);
        if (strstr(buffer, expectedMessage) == NULL) {
            fprintf(stderr, "Expected parser error containing '%s', got '%s'\n", expectedMessage, buffer);
        }
        EXPECT_TRUE(strstr(buffer, expectedMessage) != NULL);
    }

    close(fd);
    unlink(pathTemplate);
}

void test_parser_reports_error_positions(void) {
    assertParseErrorAt("x::int = 1;\nif (x\n{ ret 1; }", "expected ')' after if condition", 3, 1);
    assertParseErrorAt("x::int = 1;\nfoo(1;", "expected ')' after call arguments", 2, 6);
}

void test_parser_parses_assignment_without_symbol_lookup(void) {
    Token *tokens = NULL;
    AstNode *root;

    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "x"));
    appendToken(&tokens, makeToken(TOKEN_ASSIGN, "="));
    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "y"));
    appendToken(&tokens, makeToken(TOKEN_SEMICOLON, ";"));

    root = parseRoot(tokens);

    EXPECT_TRUE(root->type == AST_PROGRAM);
    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_ASSIGN);
    EXPECT_TRUE(root->data.program.items[0]->data.assign.target->type == AST_IDENTIFIER);
    EXPECT_STR_EQ("x", root->data.program.items[0]->data.assign.target->data.identifier.name);
    EXPECT_STR_EQ("y", root->data.program.items[0]->data.assign.value->data.identifier.name);

    astFree(root);
    freeTokenList(tokens);
}

void test_parser_parses_declaration_syntax_without_semantics(void) {
    Token *tokens = NULL;
    AstNode *root;

    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "x"));
    appendToken(&tokens, makeToken(TOKEN_DOUBLE_COLON, "::"));
    appendToken(&tokens, makeToken(TOKEN_TYPE_INT, "int"));
    appendToken(&tokens, makeToken(TOKEN_ASSIGN, "="));
    appendToken(&tokens, makeToken(TOKEN_NUMBER, "10"));
    appendToken(&tokens, makeToken(TOKEN_SEMICOLON, ";"));

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_VAR_DECL);
    EXPECT_STR_EQ("x", root->data.program.items[0]->data.var_decl.name);
    EXPECT_TRUE(root->data.program.items[0]->data.var_decl.declared_type->type == AST_TYPE_NAME);
    EXPECT_STR_EQ("int", root->data.program.items[0]->data.var_decl.declared_type->data.type_name.name);
    EXPECT_TRUE(root->data.program.items[0]->data.var_decl.initializer->type == AST_INT_LITERAL);

    astFree(root);
    freeTokenList(tokens);
}

void test_parser_parses_call_syntax_without_function_type(void) {
    Token *tokens = NULL;
    AstNode *root;

    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "print"));
    appendToken(&tokens, makeToken(TOKEN_LPAREN, "("));
    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "x"));
    appendToken(&tokens, makeToken(TOKEN_RPAREN, ")"));
    appendToken(&tokens, makeToken(TOKEN_SEMICOLON, ";"));

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_EXPR_STMT);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->type == AST_CALL);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.call.callee->type == AST_IDENTIFIER);
    EXPECT_STR_EQ("print", root->data.program.items[0]->data.expr_stmt.expression->data.call.callee->data.identifier.name);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.call.arg_count == 1);

    astFree(root);
    freeTokenList(tokens);
}

void test_parser_parses_array_index_syntax_without_array_type(void) {
    Token *tokens = NULL;
    AstNode *root;

    appendToken(&tokens, makeToken(TOKEN_IDENTIFIER, "arr"));
    appendToken(&tokens, makeToken(TOKEN_LBRACKET, "["));
    appendToken(&tokens, makeToken(TOKEN_NUMBER, "0"));
    appendToken(&tokens, makeToken(TOKEN_RBRACKET, "]"));
    appendToken(&tokens, makeToken(TOKEN_SEMICOLON, ";"));

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_EXPR_STMT);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->type == AST_ARRAY_ACCESS);
    EXPECT_STR_EQ("arr", root->data.program.items[0]->data.expr_stmt.expression->data.array_access.base->data.identifier.name);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.array_access.index_count == 1);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.array_access.indices[0]->type == AST_INT_LITERAL);

    astFree(root);
    freeTokenList(tokens);
}

void test_parser_parses_grouping_with_specific_delimiter_tokens(void) {
    Token *tokens = NULL;
    AstNode *root;

    appendToken(&tokens, makeToken(TOKEN_LPAREN, "("));
    appendToken(&tokens, makeToken(TOKEN_NUMBER, "1"));
    appendToken(&tokens, makeToken(TOKEN_PLUS, "+"));
    appendToken(&tokens, makeToken(TOKEN_NUMBER, "2"));
    appendToken(&tokens, makeToken(TOKEN_RPAREN, ")"));
    appendToken(&tokens, makeToken(TOKEN_SEMICOLON, ";"));

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_EXPR_STMT);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->type == AST_BINARY);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.binary.op == AST_BINARY_ADD);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.binary.left->data.int_literal.value == 1);
    EXPECT_TRUE(root->data.program.items[0]->data.expr_stmt.expression->data.binary.right->data.int_literal.value == 2);

    astFree(root);
    freeTokenList(tokens);
}

void test_parser_gives_and_higher_precedence_than_or(void) {
    Token *tokens = tokenizeString("a || b && c;");
    Parser parser;
    AstNode *root;
    AstNode *expr;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_EXPR_STMT);

    expr = root->data.program.items[0]->data.expr_stmt.expression;
    EXPECT_TRUE(expr->type == AST_BINARY);

    if (expr->type == AST_BINARY) {
        EXPECT_TRUE(expr->data.binary.op == AST_BINARY_OR);

        if (expr->data.binary.left != NULL) {
            EXPECT_TRUE(expr->data.binary.left->type == AST_IDENTIFIER);
            if (expr->data.binary.left->type == AST_IDENTIFIER) {
                EXPECT_STR_EQ("a", expr->data.binary.left->data.identifier.name);
            }
        }

        if (expr->data.binary.right != NULL) {
            EXPECT_TRUE(expr->data.binary.right->type == AST_BINARY);
            if (expr->data.binary.right->type == AST_BINARY) {
                EXPECT_TRUE(expr->data.binary.right->data.binary.op == AST_BINARY_AND);

                if (expr->data.binary.right->data.binary.left != NULL) {
                    EXPECT_TRUE(expr->data.binary.right->data.binary.left->type == AST_IDENTIFIER);
                    if (expr->data.binary.right->data.binary.left->type == AST_IDENTIFIER) {
                        EXPECT_STR_EQ("b", expr->data.binary.right->data.binary.left->data.identifier.name);
                    }
                }

                if (expr->data.binary.right->data.binary.right != NULL) {
                    EXPECT_TRUE(expr->data.binary.right->data.binary.right->type == AST_IDENTIFIER);
                    if (expr->data.binary.right->data.binary.right->type == AST_IDENTIFIER) {
                        EXPECT_STR_EQ("c", expr->data.binary.right->data.binary.right->data.identifier.name);
                    }
                }
            }
        }
    }

    astFree(root);
    freeTokens(tokens);
}

void test_parser_struct_parses_program_from_initialized_state(void) {
    Token *tokens = tokenizeString("x::int = 1;");
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root != NULL);
    EXPECT_TRUE(root->type == AST_PROGRAM);
    EXPECT_TRUE(root->data.program.count == 1);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_parses_float_literals(void) {
    Token *tokens = tokenizeString("x::float = 1.5;");
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_VAR_DECL);
    EXPECT_TRUE(root->data.program.items[0]->data.var_decl.initializer->type == AST_FLOAT_LITERAL);
    EXPECT_TRUE(root->data.program.items[0]->data.var_decl.initializer->data.float_literal.value == 1.5);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_copies_token_strings_into_ast(void) {
    Token *tokens = tokenizeString("name::int = 10;");
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    tokens->value[0] = 'X';
    tokens->next->next->value[0] = 'I';

    EXPECT_STR_EQ("name", root->data.program.items[0]->data.var_decl.name);
    EXPECT_STR_EQ("int", root->data.program.items[0]->data.var_decl.declared_type->data.type_name.name);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_reports_syntax_error_at_eof_without_crashing(void) {
    assertParseError("x =", "unexpected end of input in expression");
}

void test_parser_reports_specific_syntax_errors(void) {
    assertParseError("if x) { ret 1; }", "expected '(' after if");
    assertParseError("if (x { ret 1; }", "expected ')' after if condition");
    assertParseError("while x) { ret 1; }", "expected '(' after while");
    assertParseError("while (x { ret 1; }", "expected ')' after while condition");
    assertParseError("for x; x; x) { ret 1; }", "expected '(' after for");
    assertParseError("for(i = 0 i < 1; i += 1) { ret 1; }", "expected ';' after for init");
    assertParseError("for(i = 0; i < 1 i += 1) { ret 1; }", "expected ';' after for condition");
    assertParseError("for(i = 0; i < 1; i += 1 { ret 1; }", "expected ')' after for update");
    assertParseError("func f(::int) { ret 1; }", "expected parameter name");
    assertParseError("func f(a int) { ret 1; }", "expected '::' after parameter name");
    assertParseError("func (a::int) { ret 1; }", "expected function name after func");
    assertParseError("func f a::int) { ret 1; }", "expected '(' after function name");
    assertParseError("func f(a::int { ret 1; }", "expected ')' after function parameters");
    assertParseError("if (x) ret 1;", "expected '{' to start block");
    assertParseError("while (x) ret 1;", "expected '{' to start block");
    assertParseError("for(i = 0; i < 1; i += 1) ret 1;", "expected '{' to start block");
    assertParseError("func f(a::int) ret 1;", "expected '{' to start block");
    assertParseError("x", "expected ';' after statement");
    assertParseError("foo(1;", "expected ')' after call arguments");
    assertParseError("arr[1;", "expected ']' after array index");
    assertParseError("(1 + 2;", "expected ')' after grouped expression");
    assertParseError(";", "unexpected token in expression");
    assertParseError("x::= 1;", "expected type");
    assertParseError("x::Array<int = 1;", "expected '>' after array element type");
    assertParseError("x::int[1 = 0;", "expected ']' after array type size");
    assertParseError("x = ];", "unexpected token in expression");
    assertParseError("x = {1;", "expected '}' after array literal");
}

void test_parser_reports_eof_errors_in_last_token_positions(void) {
    assertParseError("foo(", "unexpected end of input in expression");
    assertParseError("arr[1", "expected ']' after array index");
    assertParseError("(1 + 2", "expected ')' after grouped expression");
    assertParseError("func f(a::int", "expected ')' after function parameters");
    assertParseError("x::", "expected type");
    assertParseError("x::Array<int", "expected '>' after array element type");
    assertParseError("x = {1", "expected '}' after array literal");
}

void test_parser_rejects_invalid_assignment_targets(void) {
    assertParseError("foo() = 1;", "invalid assignment target");
    assertParseError("a + b = 1;", "invalid assignment target");
}
