#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"
#include "../../semantic/semantic.h"

void test_semantic_starts_with_no_errors_for_empty_program(void) {
    AstNode *root = parseRootFromString("");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_rejects_incompatible_initializer_types(void) {
    AstNode *root = parseRootFromString("x::int = true;");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(result.errors.items[0].line == 1);
    EXPECT_TRUE(result.errors.items[0].column == 1);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "cannot initialize variable 'x' of type int with bool") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_duplicate_variable_in_same_scope(void) {
    AstNode *root = parseRootFromString("x::int = 1; x::int = 2;");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "duplicate declaration of 'x'") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_allows_shadowing_in_nested_scope(void) {
    AstNode *root = parseRootFromString("x::int = 1; if (true) { x::int = 2; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_duplicate_function_in_global_scope(void) {
    AstNode *root = parseRootFromString(
        "func foo(a::int) { ret a; }\n"
        "func foo(b::int) { ret b; }\n"
    );
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'foo'") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_duplicate_parameter_name(void) {
    AstNode *root = parseRootFromString("func foo(a::int, a::int) { ret a; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'a'") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_undeclared_variable_use(void) {
    AstNode *root = parseRootFromString("x = y;");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_NAME);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "use of undeclared variable 'y'") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_invalid_arithmetic_types(void) {
    AstNode *root = parseRootFromString("x::int = true + 1;");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '+' requires numeric operands") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_non_boolean_condition(void) {
    AstNode *root = parseRootFromString("if (1) { x::int = 1; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "condition must be bool") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_call_to_undeclared_function(void) {
    AstNode *root = parseRootFromString("foo(1);");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_NAME);
    EXPECT_TRUE(result.errors.items[0].line == 1);
    EXPECT_TRUE(result.errors.items[0].column == 1);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "call to undeclared function 'foo'") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_wrong_argument_count(void) {
    AstNode *root = parseRootFromString("func foo(a::int) { ret a; } foo(1, 2);");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'foo' expects 1 arguments but got 2") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_wrong_argument_type(void) {
    AstNode *root = parseRootFromString("func foo(a::int) { ret a; } foo(true);");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "argument 1 of function 'foo' expects int but got bool") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_non_integer_array_index(void) {
    AstNode *root = parseRootFromString("arr::Array<int> = {1, 2}; arr[true];");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "array index must be int") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_indexing_non_array_value(void) {
    AstNode *root = parseRootFromString("x::int = 1; x[0];");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot index non-array value of type int") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_accumulates_multiple_errors(void) {
    SemanticResult result = analyzeRootFromString(
        "x::int = true;\n"
        "y = z;\n"
        "if (1) { arr::Array<int> = {1}; arr[false]; }\n"
    );

    EXPECT_TRUE(result.errors.count == 4);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot initialize variable 'x'") != NULL);
    EXPECT_TRUE(result.errors.items[1].kind == SEMANTIC_ERROR_NAME);
    EXPECT_TRUE(strstr(result.errors.items[1].message, "use of undeclared variable 'z'") != NULL);
    EXPECT_TRUE(result.errors.items[2].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[2].message, "condition must be bool") != NULL);
    EXPECT_TRUE(result.errors.items[3].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[3].message, "array index must be int") != NULL);

    semanticResultFree(&result);
}
