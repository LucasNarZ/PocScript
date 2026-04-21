#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"
#include "../../lexer/lexer.h"

void test_tokenize_file_appends_single_eof_for_multiline_input(void) {
    Token *tokens = tokenizeFile("tests/fixtures/lexer/basic_program.ps");
    Token *cursor = tokens;
    Parser parser;
    AstNode *root;
    int eofCount = 0;

    EXPECT_TRUE(tokens != NULL);
    while (cursor != NULL) {
        if (cursor->type == TOKEN_EOF) {
            eofCount++;
        }
        cursor = cursor->next;
    }

    EXPECT_TRUE(eofCount == 1);

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);
    EXPECT_TRUE(root != NULL);
    EXPECT_TRUE(root->data.program.count == 3);

    astFree(root);
    freeTokens(tokens);
}

void test_integration_parses_input_program_ast(void) {
    char *input = readFileToString("tests/fixtures/integration/input_program.ps");
    char *expected = readFileToString("tests/fixtures/integration/input_program_ast.expected");
    AstNode *root;
    char *actual;

    EXPECT_TRUE(input != NULL);
    EXPECT_TRUE(expected != NULL);

    if (input == NULL || expected == NULL) {
        free(input);
        free(expected);
        return;
    }

    root = parseRootFromString(input);
    actual = nodeTreeToString(root);

    EXPECT_TRUE(actual != NULL);
    if (actual != NULL) {
        EXPECT_STR_EQ(expected, actual);
    }

    astFree(root);
    free(input);
    free(expected);
    free(actual);
}
