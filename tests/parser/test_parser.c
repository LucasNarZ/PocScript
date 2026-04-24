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

static void assertParsesWithAstSubstring(const char *input, const char *expectedSubstring) {
    char pathTemplate[] = "/tmp/pocscript-parser-success-XXXXXX";
    char buffer[4096];
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
        AstNode *root;
        char *astString;

        parserInit(&parser, tokens);
        root = parserParseProgram(&parser);
        astString = astToString(root);

        if (astString != NULL) {
            write(fd, astString, strlen(astString));
        }

        close(fd);
        free(astString);
        astFree(root);
        freeTokens(tokens);
        _exit(0);
    }

    close(fd);
    EXPECT_TRUE(waitpid(pid, &status, 0) == pid);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_TRUE(WEXITSTATUS(status) == 0);

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
        if (strstr(buffer, expectedSubstring) == NULL) {
            fprintf(stderr, "Expected AST containing '%s', got '%s'\n", expectedSubstring, buffer);
        }
        EXPECT_TRUE(strstr(buffer, expectedSubstring) != NULL);
    }

    close(fd);
    unlink(pathTemplate);
}

void test_parser_reports_error_positions(void) {
    assertParseErrorAt("func main() -> void {\nif (x\n{ ret 1; }\n}", "expected ')' after if condition", 3, 1);
    assertParseErrorAt("func main() -> void {\nfoo(1;\n}", "expected ')' after call arguments", 2, 6);
}

void test_parser_parses_unary_minus_expression(void) {
    assertParsesWithAstSubstring("x::int = -1;", "(UNARY)");
}

void test_parser_parses_break_and_continue_statements(void) {
    assertParsesWithAstSubstring("func main() -> void { while (true) { break; continue; } }", "(BREAK)");
    assertParsesWithAstSubstring("func main() -> void { while (true) { break; continue; } }", "(CONTINUE)");
}

void test_parser_rejects_assignment_at_file_scope(void) {
    assertParseError("x = 1;", "expected top-level declaration");
}

void test_parser_rejects_expression_statement_at_file_scope(void) {
    assertParseError("foo();", "expected top-level declaration");
}

void test_parser_rejects_if_at_file_scope(void) {
    assertParseError("if (true) { }", "expected top-level declaration");
}

void test_parser_rejects_while_at_file_scope(void) {
    assertParseError("while (true) { }", "expected top-level declaration");
}

void test_parser_rejects_for_at_file_scope(void) {
    assertParseError("for(i::int = 0; i < 1; i += 1) { }", "expected top-level declaration");
}

void test_parser_rejects_return_break_and_continue_at_file_scope(void) {
    assertParseError("ret;", "expected top-level declaration");
    assertParseError("break;", "expected top-level declaration");
    assertParseError("continue;", "expected top-level declaration");
}

void test_parser_parses_assignment_without_symbol_lookup(void) {
    Token *tokens = tokenizeString("func main() -> void { x = y; }");
    AstNode *root;
    AstNode *bodyItem;

    root = parseRoot(tokens);

    EXPECT_TRUE(root->type == AST_PROGRAM);
    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);

    bodyItem = root->data.program.items[0]->data.func_decl.body->data.block.items[0];
    EXPECT_TRUE(bodyItem->type == AST_ASSIGN);
    EXPECT_TRUE(bodyItem->data.assign.target->type == AST_IDENTIFIER);
    EXPECT_STR_EQ("x", bodyItem->data.assign.target->data.identifier.name);
    EXPECT_STR_EQ("y", bodyItem->data.assign.value->data.identifier.name);

    astFree(root);
    freeTokens(tokens);
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
    Token *tokens = tokenizeString("func main() -> void { print(x); }");
    AstNode *root;
    AstNode *exprStmt;

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);

    exprStmt = root->data.program.items[0]->data.func_decl.body->data.block.items[0];
    EXPECT_TRUE(exprStmt->type == AST_EXPR_STMT);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->type == AST_CALL);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.call.callee->type == AST_IDENTIFIER);
    EXPECT_STR_EQ("print", exprStmt->data.expr_stmt.expression->data.call.callee->data.identifier.name);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.call.arg_count == 1);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_parses_array_index_syntax_without_array_type(void) {
    Token *tokens = tokenizeString("func main() -> void { arr[0]; }");
    AstNode *root;
    AstNode *exprStmt;

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);

    exprStmt = root->data.program.items[0]->data.func_decl.body->data.block.items[0];
    EXPECT_TRUE(exprStmt->type == AST_EXPR_STMT);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->type == AST_ARRAY_ACCESS);
    EXPECT_STR_EQ("arr", exprStmt->data.expr_stmt.expression->data.array_access.base->data.identifier.name);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.array_access.index_count == 1);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.array_access.indices[0]->type == AST_INT_LITERAL);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_parses_grouping_with_specific_delimiter_tokens(void) {
    Token *tokens = tokenizeString("func main() -> void { (1 + 2); }");
    AstNode *root;
    AstNode *exprStmt;

    root = parseRoot(tokens);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);

    exprStmt = root->data.program.items[0]->data.func_decl.body->data.block.items[0];
    EXPECT_TRUE(exprStmt->type == AST_EXPR_STMT);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->type == AST_BINARY);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.binary.op == AST_BINARY_ADD);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.binary.left->data.int_literal.value == 1);
    EXPECT_TRUE(exprStmt->data.expr_stmt.expression->data.binary.right->data.int_literal.value == 2);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_gives_and_higher_precedence_than_or(void) {
    Token *tokens = tokenizeString("func main() -> void { a || b && c; }");
    Parser parser;
    AstNode *root;
    AstNode *expr;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);

    expr = root->data.program.items[0]->data.func_decl.body->data.block.items[0]->data.expr_stmt.expression;
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
    assertParseError("func main() -> void { x =", "unexpected end of input in expression");
}

void test_parser_reports_specific_syntax_errors(void) {
    assertParseError("func main() -> void { if x) { ret 1; } }", "expected '(' after if");
    assertParseError("func main() -> void { if (x { ret 1; } }", "expected ')' after if condition");
    assertParseError("func main() -> void { while x) { ret 1; } }", "expected '(' after while");
    assertParseError("func main() -> void { while (x { ret 1; } }", "expected ')' after while condition");
    assertParseError("func main() -> void { for x; x; x) { ret 1; } }", "expected '(' after for");
    assertParseError("func main() -> void { for(i = 0 i < 1; i += 1) { ret 1; } }", "expected ';' after for init");
    assertParseError("func main() -> void { for(i = 0; i < 1 i += 1) { ret 1; } }", "expected ';' after for condition");
    assertParseError("func main() -> void { for(i = 0; i < 1; i += 1 { ret 1; } }", "expected ')' after for update");
    assertParseError("func f(::int) { ret 1; }", "expected parameter name");
    assertParseError("func f(a int) { ret 1; }", "expected '::' after parameter name");
    assertParseError("func (a::int) { ret 1; }", "expected function name after func");
    assertParseError("func f a::int) { ret 1; }", "expected '(' after function name");
    assertParseError("func f(a::int { ret 1; }", "expected ')' after function parameters");
    assertParseError("func main() -> void { if (x) ret 1; }", "expected '{' to start block");
    assertParseError("func main() -> void { while (x) ret 1; }", "expected '{' to start block");
    assertParseError("func main() -> void { for(i = 0; i < 1; i += 1) ret 1; }", "expected '{' to start block");
    assertParseError("func f(a::int) -> int ret 1;", "expected '{' to start block");
    assertParseError("x", "expected top-level declaration");
    assertParseError("func main() -> void { foo(1; }", "expected ')' after call arguments");
    assertParseError("func main() -> void { arr[1; }", "expected ']' after array index");
    assertParseError("func main() -> void { (1 + 2; }", "expected ')' after grouped expression");
    assertParseError("func main() -> void { ; }", "unexpected token in expression");
    assertParseError("x::= 1;", "expected type");
    assertParseError("x::Array<int = 1;", "expected '>' after array element type");
    assertParseError("x::int[1 = 0;", "expected ']' after array type size");
    assertParseError("func main() -> void { x = ]; }", "unexpected token in expression");
    assertParseError("func main() -> void { x = {1", "expected '}' after array literal");
}

void test_parser_reports_eof_errors_in_last_token_positions(void) {
    assertParseError("func main() -> void { foo(", "unexpected end of input in expression");
    assertParseError("func main() -> void { arr[1", "expected ']' after array index");
    assertParseError("func main() -> void { (1 + 2", "expected ')' after grouped expression");
    assertParseError("func f(a::int", "expected ')' after function parameters");
    assertParseError("x::", "expected type");
    assertParseError("x::Array<int", "expected '>' after array element type");
    assertParseError("func main() -> void { x = {1", "expected '}' after array literal");
}

void test_parser_rejects_invalid_assignment_targets(void) {
    assertParseError("func main() -> void { foo() = 1; }", "invalid assignment target");
    assertParseError("func main() -> void { a + b = 1; }", "invalid assignment target");
}

void test_parser_requires_explicit_function_return_type(void) {
    assertParseError("func foo() { ret 1; }", "expected '->' after function parameters");
}

void test_parser_parses_function_return_types(void) {
    Token *tokens = tokenizeString(
        "func soma(a::int, b::int) -> int { ret a + b; }\n"
        "func logar() -> void { }"
    );
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root->data.program.count == 2);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);
    EXPECT_TRUE(root->data.program.items[0]->data.func_decl.return_type->type == AST_TYPE_NAME);
    EXPECT_TRUE(root->data.program.items[0]->data.func_decl.return_type->data.type_name.kind == AST_TYPE_INT_KIND);
    EXPECT_STR_EQ("int", root->data.program.items[0]->data.func_decl.return_type->data.type_name.name);
    EXPECT_TRUE(root->data.program.items[1]->data.func_decl.return_type->type == AST_TYPE_NAME);
    EXPECT_STR_EQ("void", root->data.program.items[1]->data.func_decl.return_type->data.type_name.name);

    astFree(root);
    freeTokens(tokens);
}

void test_parser_parses_empty_return_in_void_function(void) {
    Token *tokens = tokenizeString("func foo() -> void { ret; }");
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    EXPECT_TRUE(root->data.program.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->type == AST_FUNC_DECL);
    EXPECT_TRUE(root->data.program.items[0]->data.func_decl.body->data.block.count == 1);
    EXPECT_TRUE(root->data.program.items[0]->data.func_decl.body->data.block.items[0]->type == AST_RETURN);
    EXPECT_TRUE(root->data.program.items[0]->data.func_decl.body->data.block.items[0]->data.return_stmt.value == NULL);

    astFree(root);
    freeTokens(tokens);
}
