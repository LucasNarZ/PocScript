#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"

#include "compiler_driver.h"
#include "lexer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool commandSucceeded(const char *command) {
    return system(command) == 0;
}

static void removeIfExists(const char *path) {
    if (path != NULL) {
        unlink(path);
    }
}

static bool compileCliBinary(const char *output_path) {
    char command[8192];

    snprintf(
        command,
        sizeof(command),
        "gcc -Iinclude -Wall -g src/main.c src/compiler_driver.c src/lexer/lexer.c src/parser/parser.c src/parser/ast.c src/semantic/semantic.c src/semantic/errors.c src/semantic/types.c src/semantic/scope.c src/ir/ir_core.c src/ir/ir_instr.c src/ir/ir_module.c src/ir/ir_scope.c src/ir/ir_builder_utils.c src/ir/ir_builder_module.c src/ir/ir_builder_expr.c src/ir/ir_builder_stmt.c src/ir/ir_printer.c -o %s",
        output_path
    );

    return commandSucceeded(command);
}

void test_compiler_driver_rejects_missing_input_file(void) {
    char outputPath[] = "/tmp/pocscript-missing-output-XXXXXX.ll";
    int fd = mkstemps(outputPath, 3);
    bool ok;

    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        return;
    }

    close(fd);
    removeIfExists(outputPath);

    ok = compileSourceFileToLlvmIr("/tmp/pocscript-file-that-does-not-exist.ps", outputPath);

    EXPECT_TRUE(!ok);
    EXPECT_TRUE(access(outputPath, F_OK) != 0);
}

void test_compiler_driver_rejects_semantic_errors_without_writing_output(void) {
    char outputPath[] = "/tmp/pocscript-semantic-output-XXXXXX.ll";
    int fd = mkstemps(outputPath, 3);
    bool ok;

    EXPECT_TRUE(fd >= 0);
    if (fd < 0) {
        return;
    }

    close(fd);
    removeIfExists(outputPath);

    ok = writeLlvmIrFromStringToFile("func main() -> int { }", outputPath);

    EXPECT_TRUE(!ok);
    EXPECT_TRUE(access(outputPath, F_OK) != 0);
}

void test_compiler_driver_rejects_invalid_output_path(void) {
    bool ok = writeLlvmIrFromStringToFile(
        "func main() -> int { ret 1; }",
        "/tmp/pocscript-output-dir-that-does-not-exist/output.ll"
    );

    EXPECT_TRUE(!ok);
    EXPECT_TRUE(access("/tmp/pocscript-output-dir-that-does-not-exist/output.ll", F_OK) != 0);
}

void test_cli_binary_rejects_too_many_arguments(void) {
    char dirTemplate[] = "/tmp/pocscript-cli-XXXXXX";
    char binaryPath[256];
    char stderrPath[256];
    char command[1024];
    char *stderrText;

    EXPECT_TRUE(mkdtemp(dirTemplate) != NULL);
    if (access(dirTemplate, F_OK) != 0) {
        return;
    }

    snprintf(binaryPath, sizeof(binaryPath), "%s/compiler", dirTemplate);
    snprintf(stderrPath, sizeof(stderrPath), "%s/stderr.txt", dirTemplate);

    EXPECT_TRUE(compileCliBinary(binaryPath));
    if (access(binaryPath, X_OK) != 0) {
        removeIfExists(stderrPath);
        rmdir(dirTemplate);
        return;
    }

    snprintf(command, sizeof(command), "%s a.ps b.ll c.extra 2> %s", binaryPath, stderrPath);
    EXPECT_TRUE(!commandSucceeded(command));

    stderrText = readFileToString(stderrPath);
    EXPECT_TRUE(stderrText != NULL);
    if (stderrText != NULL) {
        EXPECT_TRUE(strstr(stderrText, "usage:") != NULL);
        free(stderrText);
    }

    removeIfExists(binaryPath);
    removeIfExists(stderrPath);
    rmdir(dirTemplate);
}

void test_cli_binary_writes_ir_for_valid_input_file(void) {
    char dirTemplate[] = "/tmp/pocscript-cli-compile-XXXXXX";
    char binaryPath[256];
    char inputPath[256];
    char outputPath[256];
    char command[1024];
    FILE *inputFile;
    char *llvm;

    EXPECT_TRUE(mkdtemp(dirTemplate) != NULL);
    if (access(dirTemplate, F_OK) != 0) {
        return;
    }

    snprintf(binaryPath, sizeof(binaryPath), "%s/compiler", dirTemplate);
    snprintf(inputPath, sizeof(inputPath), "%s/input.ps", dirTemplate);
    snprintf(outputPath, sizeof(outputPath), "%s/output.ll", dirTemplate);

    EXPECT_TRUE(compileCliBinary(binaryPath));
    if (access(binaryPath, X_OK) != 0) {
        removeIfExists(outputPath);
        rmdir(dirTemplate);
        return;
    }

    inputFile = fopen(inputPath, "w");
    EXPECT_TRUE(inputFile != NULL);
    if (inputFile == NULL) {
        removeIfExists(binaryPath);
        rmdir(dirTemplate);
        return;
    }

    fputs("func main() -> int { ret 11; }", inputFile);
    fclose(inputFile);

    snprintf(command, sizeof(command), "%s %s %s", binaryPath, inputPath, outputPath);
    EXPECT_TRUE(commandSucceeded(command));

    llvm = readFileToString(outputPath);
    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "define i32 @main()") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i32 11") != NULL);
        free(llvm);
    }

    removeIfExists(binaryPath);
    removeIfExists(inputPath);
    removeIfExists(outputPath);
    rmdir(dirTemplate);
}
