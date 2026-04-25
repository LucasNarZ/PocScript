#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"
#include "../../semantic/semantic.h"

static AstNode *makeProgramWithSingleNode(AstNode *node) {
    AstNode *program = astNewNode(AST_PROGRAM);
    astAppendNode(&program->data.program.items, &program->data.program.count, node);
    return program;
}

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
    AstNode *root = parseRootFromString("func main() -> void { x::int = 1; if (true) { x::int = 2; } }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_duplicate_function_in_global_scope(void) {
    AstNode *root = parseRootFromString(
        "func foo(a::int) -> int { ret a; }\n"
        "func foo(b::int) -> int { ret b; }\n"
    );
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'foo'") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_duplicate_parameter_name(void) {
    AstNode *root = parseRootFromString("func foo(a::int, a::int) -> int { ret a; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'a'") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_undeclared_variable_use(void) {
    AstNode *root = parseRootFromString("func main() -> void { x = y; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_NAME);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "use of undeclared variable 'y'") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_invalid_arithmetic_types(void) {
    AstNode *root = parseRootFromString("func main() -> void { x::int = true + 1; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '+' requires numeric operands") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_non_boolean_condition(void) {
    AstNode *root = parseRootFromString("func main() -> void { if (1) { x::int = 1; } }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "condition must be bool") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_call_to_undeclared_function(void) {
    AstNode *root = parseRootFromString("func main() -> void {\n    foo(1);\n}");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_NAME);
    EXPECT_TRUE(result.errors.items[0].line == 2);
    EXPECT_TRUE(result.errors.items[0].column == 5);
    EXPECT_TRUE(strcmp(result.errors.items[0].message, "call to undeclared function 'foo'") == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_accepts_builtin_print_functions(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void {\n"
        "    printString(\"hi\");\n"
        "    printInt(42);\n"
        "    ret;\n"
        "}"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_redeclaration_of_builtin_print_function(void) {
    SemanticResult result = analyzeRootFromString(
        "func printInt(value::int) -> void { ret; }\n"
        "func main() -> void { ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'printInt'") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_reports_wrong_argument_count(void) {
    AstNode *root = parseRootFromString("func foo(a::int) -> int { ret a; } func main() -> void { foo(1, 2); }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'foo' expects 1 arguments but got 2") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_wrong_argument_type(void) {
    AstNode *root = parseRootFromString("func foo(a::int) -> int { ret a; } func main() -> void { foo(true); }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "argument 1 of function 'foo' expects int but got bool") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_non_integer_array_index(void) {
    AstNode *root = parseRootFromString("func main() -> void { arr::Array<int> = {1, 2}; arr[true]; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "array index must be int") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_indexing_non_array_value(void) {
    AstNode *root = parseRootFromString("func main() -> void { x::int = 1; x[0]; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
    EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot index non-array value of type int") != NULL);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_accumulates_multiple_errors(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void {\n"
        "x::int = true;\n"
        "y = z;\n"
        "if (1) { arr::Array<int> = {1}; arr[false]; }\n"
        "}\n"
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

void test_semantic_rejects_initializing_int_with_void_call(void) {
    AstNode *root = parseRootFromString(
        "func logar() -> void { }\n"
        "func main() -> void { x::int = logar(); }"
    );
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot initialize variable 'x' of type int with void") != NULL);
    }

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_rejects_non_void_function_without_return(void) {
    SemanticResult result = analyzeRootFromString("func soma(a::int, b::int) -> int { }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'soma' with return type int must return a value") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_incompatible_function_return_type(void) {
    SemanticResult result = analyzeRootFromString("func foo() -> int { ret true; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "return type of function 'foo' expects int but got bool") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_value_return_inside_void_function(void) {
    SemanticResult result = analyzeRootFromString("func foo() -> void { ret 1; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'foo' with return type void cannot return a value") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_reports_return_statement_outside_function(void) {
    AstNode *program = astNewNode(AST_PROGRAM);
    AstNode *returnNode = astNewNode(AST_RETURN);
    AstNode *literal = astNewNode(AST_INT_LITERAL);
    SemanticResult result;

    literal->data.int_literal.value = 1;
    returnNode->data.return_stmt.value = literal;
    astAppendNode(&program->data.program.items, &program->data.program.count, returnNode);

    result = semanticAnalyze(program);

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "return statement outside function") != NULL);
    }

    semanticResultFree(&result);
    astFree(program);
}

void test_semantic_accepts_matching_function_return_type(void) {
    SemanticResult result = analyzeRootFromString("func foo() -> int { ret 1; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_accepts_empty_return_inside_void_function(void) {
    SemanticResult result = analyzeRootFromString("func foo() -> void { ret; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_empty_return_inside_non_void_function(void) {
    SemanticResult result = analyzeRootFromString("func foo() -> int { ret; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'foo' with return type int cannot use empty return") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_reports_empty_return_outside_function(void) {
    AstNode *program = astNewNode(AST_PROGRAM);
    AstNode *returnNode = astNewNode(AST_RETURN);
    SemanticResult result;

    astAppendNode(&program->data.program.items, &program->data.program.count, returnNode);

    result = semanticAnalyze(program);

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "return statement outside function") != NULL);
    }

    semanticResultFree(&result);
    astFree(program);
}

void test_semantic_accepts_unary_minus_for_numeric_operand(void) {
    AstNode *root = parseRootFromString("func main() -> void { x::int = -1; }");
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_break_outside_loop(void) {
    AstNode *root = makeProgramWithSingleNode(astNewNode(AST_BREAK));
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "break statement outside loop") != NULL);
    }

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_reports_continue_outside_loop(void) {
    AstNode *root = makeProgramWithSingleNode(astNewNode(AST_CONTINUE));
    SemanticResult result = semanticAnalyze(root);

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "continue statement outside loop") != NULL);
    }

    semanticResultFree(&result);
    astFree(root);
}

void test_semantic_accepts_literal_global_initializers(void) {
    SemanticResult result = analyzeRootFromString(
        "x::int = 1;\n"
        "y::float = 1.5;\n"
        "flag::bool = true;\n"
        "letter::char = \"a\";\n"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_computed_global_initializer(void) {
    SemanticResult result = analyzeRootFromString("x::int = 1 + 2;");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "global initializer") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_identifier_global_initializer(void) {
    SemanticResult result = analyzeRootFromString(
        "base::int = 1;\n"
        "x::int = base;"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "global initializer") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_break_and_continue_inside_while(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { while (true) { break; continue; } }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_accepts_break_and_continue_inside_for(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { for(i::int = 0; i < 10; i += 1) { break; continue; } }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}
