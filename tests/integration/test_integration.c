#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"
#include "lexer.h"

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

void test_integration_parses_loop_control_and_unary_ast(void) {
    char *input = readFileToString("tests/fixtures/integration/loop_control_unary.ps");
    char *expected = readFileToString("tests/fixtures/integration/loop_control_unary_ast.expected");
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

void test_integration_emits_ir_for_globals_function_calls_and_control_flow(void) {
    char *llvm = emitLlvmIrFromString(
        "counter::int = 1; "
        "func addOne(value::int) -> int { ret value + 1; } "
        "func main() -> int { next::int = addOne(counter); if (next > 1) { ret next; } ret 0; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "@counter = global i32 1") != NULL);
        EXPECT_TRUE(strstr(llvm, "call i32 @addOne") != NULL);
        EXPECT_TRUE(strstr(llvm, "icmp sgt") != NULL);
        EXPECT_TRUE(strstr(llvm, "br i1") != NULL);
        free(llvm);
    }
}

void test_integration_emits_ir_for_single_dimension_array_access(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { arr::int[3] = {1, 2, 3}; picked::int = arr[1]; ret picked; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "alloca [3 x i32]") != NULL);
        EXPECT_TRUE(strstr(llvm, "getelementptr inbounds [3 x i32]") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i32") != NULL);
        free(llvm);
    }
}

void test_integration_emits_ir_for_runtime_print_calls(void) {
    char *llvm = emitLlvmIrFromString(
        "extern func printString(value::*char) -> void;\n"
        "extern func printInt(value::int) -> void;\n"
        "func main() -> void { printString(\"hi\"); printInt(42); ret; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "declare void @printString(i8*)") != NULL);
        EXPECT_TRUE(strstr(llvm, "declare void @printInt(i32)") != NULL);
        EXPECT_TRUE(strstr(llvm, "call void @printString") != NULL);
        EXPECT_TRUE(strstr(llvm, "call void @printInt") != NULL);
        free(llvm);
    }
}
