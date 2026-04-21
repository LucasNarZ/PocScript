#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int test_failures;
extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define TEST_COLOR_RED "\x1b[31m"
#define TEST_COLOR_GREEN "\x1b[32m"
#define TEST_COLOR_YELLOW "\x1b[33m"
#define TEST_COLOR_RESET "\x1b[0m"

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            test_failures++; \
        } \
    } while (0)

#define EXPECT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            fprintf(stderr, "FAIL %s:%d: expected '%s' got '%s'\n", __FILE__, __LINE__, (expected), (actual)); \
            test_failures++; \
        } \
    } while (0)

#define RUN_TEST(test_fn, test_file) \
    do { \
        int failures_before = test_failures; \
        tests_run++; \
        test_fn(); \
        if (test_failures == failures_before) { \
            tests_passed++; \
            printf(TEST_COLOR_GREEN "[PASS]" TEST_COLOR_RESET " %s\n", #test_fn); \
        } else { \
            tests_failed++; \
            printf(TEST_COLOR_RED "[FAIL]" TEST_COLOR_RESET " %s\n", #test_fn); \
        } \
    } while (0)

void test_lexer_tokenizes_double_colon_and_types(void);
void test_lexer_tokenizes_keywords_and_assignments(void);
void test_lexer_fixture_basic_program(void);
void test_lexer_tokenizes_specific_delimiters(void);
void test_lexer_tracks_line_and_column(void);
void test_get_tokens_preserves_position_across_calls(void);
void test_lexer_struct_tracks_position_and_tokens(void);
void test_lexer_struct_preserves_scan_state(void);
void test_parser_parses_assignment_without_symbol_lookup(void);
void test_parser_parses_declaration_syntax_without_semantics(void);
void test_parser_parses_call_syntax_without_function_type(void);
void test_parser_parses_array_index_syntax_without_array_type(void);
void test_parser_parses_grouping_with_specific_delimiter_tokens(void);
void test_parser_struct_parses_program_from_initialized_state(void);
void test_parser_parses_float_literals(void);
void test_parser_copies_token_strings_into_ast(void);
void test_parser_reports_syntax_error_at_eof_without_crashing(void);
void test_parser_reports_specific_syntax_errors(void);
void test_parser_reports_eof_errors_in_last_token_positions(void);
void test_parser_reports_error_positions(void);
void test_tokenize_file_appends_single_eof_for_multiline_input(void);
void test_integration_parses_input_program_ast(void);

#endif
