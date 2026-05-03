#include "../helpers/test_helpers.h"
#include "../helpers/stdlib_test_support.h"

void test_stdlib_strlen_and_puts_emit_expected_output(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func strlen(str::*char) -> int;\n"
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int {\n"
        "    if (strlen(\"abc\") == 3) {\n"
        "        puts(\"ok\");\n"
        "        ret 0;\n"
        "    }\n"
        "    puts(\"bad\");\n"
        "    ret 1;\n"
        "}\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("ok", output);
        free(output);
    }
}

void test_stdlib_strcmp_distinguishes_equal_and_prefix_values(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func strcmp(a::*char, b::*char) -> int;\n"
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int {\n"
        "    if (strcmp(\"abc\", \"abc\") == 0 && strcmp(\"abc\", \"abd\") < 0 && strcmp(\"abd\", \"abc\") > 0) {\n"
        "        puts(\"ok\");\n"
        "        ret 0;\n"
        "    }\n"
        "    puts(\"bad\");\n"
        "    ret 1;\n"
        "}\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("ok", output);
        free(output);
    }
}

void test_stdlib_strcpy_copies_source_buffer(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func strcpy(dest::*char, src::*char) -> *char;\n"
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int {\n"
        "    buf::char[8] = {'x', 'x', 'x', 'x', '\\0', '\\0', '\\0', '\\0'};\n"
        "    strcpy(&buf[0], \"hey\");\n"
        "    puts(&buf[0]);\n"
        "    ret 0;\n"
        "}\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("hey", output);
        free(output);
    }
}

void test_stdlib_memset_and_memcpy_preserve_buffer_content(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func memcpy(dest::*char, src::*char, n::int) -> *char;\n"
        "extern func memset(dest::*char, value::int, n::int) -> *char;\n"
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int {\n"
        "    src::char[4] = {'a', 'b', 'c', '\\0'};\n"
        "    dst::char[4] = {'x', 'x', 'x', '\\0'};\n"
        "    memset(&dst[0], 122, 3);\n"
        "    memcpy(&dst[0], &src[0], 4);\n"
        "    puts(&dst[0]);\n"
        "    ret 0;\n"
        "}\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("abc", output);
        free(output);
    }
}

void test_stdlib_strncpy_truncates_when_limit_is_shorter_than_source(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func strncpy(dest::*char, src::*char, n::int) -> *char;\n"
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int {\n"
        "    buf::char[6] = {'x', 'x', 'x', 'x', '\\0', '\\0'};\n"
        "    strncpy(&buf[0], \"planet\", 3);\n"
        "    buf[3] = '\\0';\n"
        "    puts(&buf[0]);\n"
        "    ret 0;\n"
        "}\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("pla", output);
        free(output);
    }
}

void test_stdlib_puts_does_not_append_newline(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int { puts(\"line\"); ret 0; }\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("line", output);
        free(output);
    }
}

void test_stdlib_puts_interprets_backslash_n_as_newline(void) {
    char *output = compileAndRunStdlibProgram(
        "extern func puts(str::*char) -> int;\n"
        "func main() -> int { puts(\"a\\nb\"); ret 0; }\n"
    );

    EXPECT_TRUE(output != NULL);
    if (output != NULL) {
        EXPECT_STR_EQ("a\nb", output);
        free(output);
    }
}
