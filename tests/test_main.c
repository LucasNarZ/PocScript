#include "helpers/test_helpers.h"

int test_failures = 0;
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

int main(void) {
    RUN_TEST(test_lexer_tokenizes_double_colon_and_types, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_tokenizes_keywords_and_assignments, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_fixture_basic_program, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_tokenizes_specific_delimiters, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_tracks_line_and_column, "tests/lexer/test_lexer.c");
    RUN_TEST(test_get_tokens_preserves_position_across_calls, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_struct_tracks_position_and_tokens, "tests/lexer/test_lexer.c");
    RUN_TEST(test_lexer_struct_preserves_scan_state, "tests/lexer/test_lexer.c");
    RUN_TEST(test_parser_parses_assignment_without_symbol_lookup, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_parses_declaration_syntax_without_semantics, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_parses_call_syntax_without_function_type, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_parses_array_index_syntax_without_array_type, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_parses_grouping_with_specific_delimiter_tokens, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_struct_parses_program_from_initialized_state, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_parses_float_literals, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_copies_token_strings_into_ast, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_reports_syntax_error_at_eof_without_crashing, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_reports_specific_syntax_errors, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_reports_eof_errors_in_last_token_positions, "tests/parser/test_parser.c");
    RUN_TEST(test_parser_reports_error_positions, "tests/parser/test_parser.c");
    RUN_TEST(test_semantic_starts_with_no_errors_for_empty_program, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_rejects_incompatible_initializer_types, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_duplicate_variable_in_same_scope, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_allows_shadowing_in_nested_scope, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_duplicate_function_in_global_scope, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_duplicate_parameter_name, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_undeclared_variable_use, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_invalid_arithmetic_types, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_non_boolean_condition, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_call_to_undeclared_function, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_wrong_argument_count, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_wrong_argument_type, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_non_integer_array_index, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_reports_indexing_non_array_value, "tests/semantic/test_semantic.c");
    RUN_TEST(test_semantic_accumulates_multiple_errors, "tests/semantic/test_semantic.c");
    RUN_TEST(test_tokenize_file_appends_single_eof_for_multiline_input, "tests/integration/test_integration.c");
    RUN_TEST(test_integration_parses_input_program_ast, "tests/integration/test_integration.c");

    printf("\n");
    printf("Tests run: %d\n", tests_run);
    printf(TEST_COLOR_GREEN "Passed: %d\n" TEST_COLOR_RESET, tests_passed);
    if (tests_failed != 0) {
        printf(TEST_COLOR_RED "Failed: %d\n" TEST_COLOR_RESET, tests_failed);
    } else {
        printf(TEST_COLOR_GREEN "Failed: %d\n" TEST_COLOR_RESET, tests_failed);
    }

    if (test_failures != 0) {
        fprintf(stderr, TEST_COLOR_RED "%d assertions failed\n" TEST_COLOR_RESET, test_failures);
        return 1;
    }

    printf(TEST_COLOR_GREEN "all tests passed\n" TEST_COLOR_RESET);
    return 0;
}
