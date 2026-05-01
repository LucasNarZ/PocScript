#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"
#include "semantic.h"

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
        "extern func printString(value::string) -> void;\n"
        "extern func printInt(value::int) -> void;\n"
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
        "extern func printInt(value::int) -> void;\n"
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

void test_semantic_accepts_extern_function_declarations(void) {
    SemanticResult result = analyzeRootFromString(
        "extern func printString(value::string) -> void;\n"
        "extern func printInt(value::int) -> void;\n"
        "func main() -> void {\n"
        "    printString(\"hi\");\n"
        "    printInt(42);\n"
        "    ret;\n"
        "}"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_duplicate_extern_function_declaration(void) {
    SemanticResult result = analyzeRootFromString(
        "extern func printInt(value::int) -> void;\n"
        "extern func printInt(value::int) -> void;\n"
        "func main() -> void { ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DECLARATION);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "duplicate declaration of 'printInt'") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_conflict_between_extern_and_defined_function(void) {
    SemanticResult result = analyzeRootFromString(
        "extern func printInt(value::int) -> void;\n"
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

void test_semantic_validates_extern_function_argument_types(void) {
    SemanticResult result = analyzeRootFromString(
        "extern func printInt(value::int) -> void;\n"
        "func main() -> void { printInt(true); ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "argument 1 of function 'printInt' expects int but got bool") != NULL);
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

void test_semantic_accepts_sized_array_argument_for_matching_parameter(void) {
    SemanticResult result = analyzeRootFromString(
        "func first(values::Array<int>[3]) -> int { ret values[0]; } "
        "func main() -> int { data::Array<int>[3] = {1, 2, 3}; ret first(data); }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
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

void test_semantic_rejects_incompatible_assignment_through_array_access(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::Array<int>[2] = {1, 2}; arr[0] = true; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "assignment types are incompatible") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_assignment_through_nested_array_access(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { matrix::Array<Array<int>[2]>[2] = {{1, 2}, {3, 4}}; matrix[0][1] = 9; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
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

void test_semantic_rejects_non_void_function_with_if_missing_else_return(void) {
    SemanticResult result = analyzeRootFromString(
        "func foo(flag::bool) -> int { if (flag) { ret 1; } }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "function 'foo' with return type int must return a value") != NULL);
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

void test_semantic_accepts_if_else_when_both_branches_return(void) {
    SemanticResult result = analyzeRootFromString(
        "func foo(flag::bool) -> int { if (flag) { ret 1; } else { ret 2; } }"
    );

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

void test_semantic_reports_unreachable_code_after_returning_if_else(void) {
    SemanticResult result = analyzeRootFromString(
        "func foo(flag::bool) -> int { if (flag) { ret 1; } else { ret 2; } ret 3; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DEVELOPER);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "unreachable code") != NULL);
    }

    semanticResultFree(&result);
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
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DEVELOPER);
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
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_DEVELOPER);
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

void test_semantic_accepts_logical_and_with_bool_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = true && false; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_logical_and_with_non_bool_operand(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = true && 1; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "logical operators require bool operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_logical_or_with_bool_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = true || false; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_logical_or_with_non_bool_operand(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = false || 1; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "logical operators require bool operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_equality_with_compatible_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { same::bool = 1 == 1; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_equality_with_incompatible_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { same::bool = 1 == true; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "comparison requires compatible operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_inequality_with_compatible_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { diff::bool = 1 != 2; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_inequality_with_incompatible_operands(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { diff::bool = 1 != true; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "comparison requires compatible operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_unary_not_for_bool_operand(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = !false; }");

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_unary_not_for_non_bool_operand(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { flag::bool = !1; }");

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(result.errors.items[0].kind == SEMANTIC_ERROR_TYPE);
        EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '!' requires bool operand") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_array_literal_with_incompatible_element_types(void) {
    SemanticResult result = analyzeRootFromString("func main() -> void { arr::Array<int> = {1, true}; }");

    EXPECT_TRUE(result.errors.count == 2);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "array literal elements must have compatible types") != NULL
            || strstr(result.errors.items[1].message, "array literal elements must have compatible types") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_address_of_variable(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { x::int = 1; p::*int = &x; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_accepts_dereference_read_and_write(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { x::int = 1; p::*int = &x; y::int = *p; *p = 2; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_address_of_non_lvalue(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { x::int = 1; p::*int = &(x + 1); ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "address-of requires an addressable expression") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_dereference_of_non_pointer(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { x::int = 1; y::int = *x; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "cannot dereference non-pointer value") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_assignment_through_pointer_with_wrong_type(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { x::int = 1; p::*int = &x; *p = true; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "assignment expects int but got bool") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_pointer_addition_with_int_offset(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::int[4] = {10, 20, 30, 40}; p::*int = &arr[0]; q::*int = p + 2; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_accepts_pointer_subtraction_with_int_offset(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::int[4] = {10, 20, 30, 40}; p::*int = &arr[2]; q::*int = p - 1; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}

void test_semantic_rejects_integer_plus_pointer(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::int[4] = {10, 20, 30, 40}; p::*int = &arr[0]; q::*int = 1 + p; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '+' requires numeric operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_pointer_plus_pointer(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::int[4] = {10, 20, 30, 40}; p::*int = &arr[0]; q::*int = &arr[1]; r::*int = p + q; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '+' requires numeric operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_pointer_minus_pointer(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { arr::int[4] = {10, 20, 30, 40}; p::*int = &arr[2]; q::*int = &arr[1]; r::*int = p - q; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '-' requires numeric operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_rejects_void_pointer_arithmetic(void) {
    SemanticResult result = analyzeRootFromString(
        "func main(value::*void) -> void { next::*void = value + 1; ret; }"
    );

    EXPECT_TRUE(result.errors.count == 1);
    if (result.errors.count > 0) {
        EXPECT_TRUE(strstr(result.errors.items[0].message, "operator '+' requires numeric operands") != NULL);
    }

    semanticResultFree(&result);
}

void test_semantic_accepts_nested_array_literal_with_matching_shapes(void) {
    SemanticResult result = analyzeRootFromString(
        "func main() -> void { matrix::Array<Array<int>> = {{1, 2}, {3, 4}}; }"
    );

    EXPECT_TRUE(result.errors.count == 0);

    semanticResultFree(&result);
}
