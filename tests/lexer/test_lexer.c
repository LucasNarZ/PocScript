#include "../helpers/test_helpers.h"
#include "lexer.h"

void test_lexer_tokenizes_double_colon_and_types(void) {
    Token *tokens = tokenizeString("x::int = 10;");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("x", tokens->value);
    EXPECT_STR_EQ("DOUBLE_COLON", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("TYPE_INT", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("ASSIGN", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("NUMBER", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("SEMICOLON", tokenTypeName(tokens->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_tokenizes_keywords_and_assignments(void) {
    Token *tokens = tokenizeString("for(i += 1){ ret true; }");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("KW_FOR", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("LPAREN", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("PLUS_ASSIGN", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("NUMBER", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("RPAREN", tokenTypeName(tokens->next->next->next->next->next->type));
    EXPECT_STR_EQ("LBRACE", tokenTypeName(tokens->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("KW_RET", tokenTypeName(tokens->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("BOOL", tokenTypeName(tokens->next->next->next->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_fixture_basic_program(void) {
    char *actual = tokenizeFileToString("tests/fixtures/lexer/basic_program.ps");
    char *expected = readFileToString("tests/fixtures/lexer/basic_program.expected");

    EXPECT_TRUE(actual != NULL);
    EXPECT_TRUE(expected != NULL);
    EXPECT_STR_EQ(expected, actual);

    free(actual);
    free(expected);
}

void test_lexer_tokenizes_specific_delimiters(void) {
    Token *tokens = tokenizeString("fn(a[0]);");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("LPAREN", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("LBRACKET", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("NUMBER", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("RBRACKET", tokenTypeName(tokens->next->next->next->next->next->type));
    EXPECT_STR_EQ("RPAREN", tokenTypeName(tokens->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("SEMICOLON", tokenTypeName(tokens->next->next->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_tracks_line_and_column(void) {
    Token *tokens = tokenizeString("x::int = 10;\n    flag::bool = true;\n");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_TRUE(tokens->line == 1);
    EXPECT_TRUE(tokens->column == 1);
    EXPECT_TRUE(tokens->next->line == 1);
    EXPECT_TRUE(tokens->next->column == 2);
    EXPECT_TRUE(tokens->next->next->line == 1);
    EXPECT_TRUE(tokens->next->next->column == 4);

    EXPECT_TRUE(tokens->next->next->next->next->next->next->line == 2);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->column == 5);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->next->next->next->next->line == 2);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->next->next->next->next->column == 18);

    freeTokens(tokens);
}

void test_get_tokens_preserves_position_across_calls(void) {
    test_lexer_struct_preserves_scan_state();
}

void test_lexer_struct_tracks_position_and_tokens(void) {
    char input[] = "x::int = 10;\n    y::int = 3;\n";
    Lexer lexer;
    Token *tokens;

    lexerInit(&lexer, input);
    lexerScan(&lexer);
    tokens = lexerTakeTokens(&lexer);

    EXPECT_TRUE(tokens != NULL);
    EXPECT_TRUE(tokens->line == 1);
    EXPECT_TRUE(tokens->column == 1);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->line == 2);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->column == 5);

    freeTokens(tokens);
}

void test_lexer_struct_preserves_scan_state(void) {
    char input[] = "x::int = 10;\n    flag::bool = true;\n";
    Lexer lexer;
    Token *tokens;
    Token *current;

    lexerInit(&lexer, input);
    lexerScan(&lexer);
    tokens = lexerTakeTokens(&lexer);

    current = tokens;
    while (current != NULL && !(current->line == 2 && current->type == TOKEN_IDENTIFIER)) {
        current = current->next;
    }

    EXPECT_TRUE(current != NULL);
    EXPECT_TRUE(current->column == 5);

    freeTokens(tokens);
}

void test_lexer_tokenizes_logical_and_equality_operators(void) {
    Token *tokens = tokenizeString("flag = a == b && c != d || !done;");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("ASSIGN", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("EQ_EQ", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("AND_AND", tokenTypeName(tokens->next->next->next->next->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("NOT_EQ", tokenTypeName(tokens->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("OR_OR", tokenTypeName(tokens->next->next->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("BANG", tokenTypeName(tokens->next->next->next->next->next->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_tokenizes_break_and_continue_keywords_together(void) {
    Token *tokens = tokenizeString("while(true){ break; continue; }");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("KW_WHILE", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("LPAREN", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("BOOL", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("RPAREN", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("LBRACE", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("KW_BREAK", tokenTypeName(tokens->next->next->next->next->next->type));
    EXPECT_STR_EQ("SEMICOLON", tokenTypeName(tokens->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("KW_CONTINUE", tokenTypeName(tokens->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("SEMICOLON", tokenTypeName(tokens->next->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("RBRACE", tokenTypeName(tokens->next->next->next->next->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_tokenizes_extern_function_declaration(void) {
    Token *tokens = tokenizeString("extern func printInt(value::int) -> void;");

    EXPECT_TRUE(tokens != NULL);
    EXPECT_STR_EQ("KW_EXTERN", tokenTypeName(tokens->type));
    EXPECT_STR_EQ("KW_FUNC", tokenTypeName(tokens->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->type));
    EXPECT_STR_EQ("LPAREN", tokenTypeName(tokens->next->next->next->type));
    EXPECT_STR_EQ("IDENTIFIER", tokenTypeName(tokens->next->next->next->next->type));
    EXPECT_STR_EQ("DOUBLE_COLON", tokenTypeName(tokens->next->next->next->next->next->type));
    EXPECT_STR_EQ("TYPE_INT", tokenTypeName(tokens->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("RPAREN", tokenTypeName(tokens->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("ARROW", tokenTypeName(tokens->next->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("TYPE_VOID", tokenTypeName(tokens->next->next->next->next->next->next->next->next->next->type));
    EXPECT_STR_EQ("SEMICOLON", tokenTypeName(tokens->next->next->next->next->next->next->next->next->next->next->type));

    freeTokens(tokens);
}

void test_lexer_tokenizes_address_of_operator(void) {
    Token *tokens = tokenizeString("p::*int = &x;");

    EXPECT_TRUE(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    EXPECT_TRUE(tokens->type == TOKEN_IDENTIFIER);
    EXPECT_TRUE(tokens->next->type == TOKEN_DOUBLE_COLON);
    EXPECT_TRUE(tokens->next->next->type == TOKEN_STAR);
    EXPECT_TRUE(tokens->next->next->next->type == TOKEN_TYPE_INT);
    EXPECT_TRUE(tokens->next->next->next->next->type == TOKEN_ASSIGN);
    EXPECT_TRUE(tokens->next->next->next->next->next->type == TOKEN_AMPERSAND);
    EXPECT_TRUE(tokens->next->next->next->next->next->next->type == TOKEN_IDENTIFIER);

    freeTokens(tokens);
}

void test_lexer_tokenizes_char_literals(void) {
    Token *tokens = tokenizeString("letter::char = 'a'; nul::char = '\\0';");
    Token *current = tokens;

    EXPECT_TRUE(tokens != NULL);
    if (tokens == NULL) {
        return;
    }

    while (current != NULL && current->type != TOKEN_ASSIGN) {
        current = current->next;
    }

    EXPECT_TRUE(current != NULL);
    EXPECT_TRUE(current != NULL && current->next != NULL);
    if (current != NULL && current->next != NULL) {
        EXPECT_TRUE(current->next->type == TOKEN_CHAR_LITERAL);
        EXPECT_STR_EQ("'a'", current->next->value);
    }

    current = tokens;
    while (current != NULL && !(current->type == TOKEN_IDENTIFIER && strcmp(current->value, "nul") == 0)) {
        current = current->next;
    }

    EXPECT_TRUE(current != NULL);
    EXPECT_TRUE(current != NULL && current->next != NULL && current->next->next != NULL && current->next->next->next != NULL && current->next->next->next->next != NULL);
    if (current != NULL && current->next != NULL && current->next->next != NULL && current->next->next->next != NULL && current->next->next->next->next != NULL) {
        EXPECT_TRUE(current->next->next->next->next->type == TOKEN_CHAR_LITERAL);
        EXPECT_STR_EQ("'\\0'", current->next->next->next->next->value);
    }

    freeTokens(tokens);
}
