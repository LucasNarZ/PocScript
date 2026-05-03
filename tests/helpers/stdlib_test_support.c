#include "stdlib_test_support.h"

#include "compiler_driver.h"
#include "lexer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool runCommand(const char *command) {
    return system(command) == 0;
}

static void removeIfExists(const char *path) {
    if (path != NULL) {
        unlink(path);
    }
}

char *compileAndRunStdlibProgram(const char *source) {
    char dirTemplate[] = "/tmp/pocscript-stdlib-XXXXXX";
    char sourcePath[256];
    char inputLl[256];
    char inputObj[256];
    char stringLl[256];
    char stringObj[256];
    char memoryLl[256];
    char memoryObj[256];
    char ioLl[256];
    char ioObj[256];
    char runtimeObj[256];
    char startObj[256];
    char outputExe[256];
    char stdoutPath[256];
    char command[8192];
    FILE *file;
    char *captured = NULL;
    long size;

    if (mkdtemp(dirTemplate) == NULL) {
        return NULL;
    }

    snprintf(sourcePath, sizeof(sourcePath), "%s/input.ps", dirTemplate);
    snprintf(inputLl, sizeof(inputLl), "%s/input.ll", dirTemplate);
    snprintf(inputObj, sizeof(inputObj), "%s/input.o", dirTemplate);
    snprintf(stringLl, sizeof(stringLl), "%s/string.ll", dirTemplate);
    snprintf(stringObj, sizeof(stringObj), "%s/string.o", dirTemplate);
    snprintf(memoryLl, sizeof(memoryLl), "%s/memory.ll", dirTemplate);
    snprintf(memoryObj, sizeof(memoryObj), "%s/memory.o", dirTemplate);
    snprintf(ioLl, sizeof(ioLl), "%s/io.ll", dirTemplate);
    snprintf(ioObj, sizeof(ioObj), "%s/io.o", dirTemplate);
    snprintf(runtimeObj, sizeof(runtimeObj), "%s/runtime.o", dirTemplate);
    snprintf(startObj, sizeof(startObj), "%s/start.o", dirTemplate);
    snprintf(outputExe, sizeof(outputExe), "%s/output", dirTemplate);
    snprintf(stdoutPath, sizeof(stdoutPath), "%s/stdout.txt", dirTemplate);

    file = fopen(sourcePath, "w");
    if (file == NULL) {
        return NULL;
    }

    fputs(source, file);
    fclose(file);

    if (!compileSourceFileToLlvmIr(sourcePath, inputLl)
            || !compileSourceFileToLlvmIr("runtime/lib/string.ps", stringLl)
            || !compileSourceFileToLlvmIr("runtime/lib/memory.ps", memoryLl)
            || !compileSourceFileToLlvmIr("runtime/lib/io.ps", ioLl)) {
        return NULL;
    }

    snprintf(
        command,
        sizeof(command),
        "clang -Wno-override-module -c %s -o %s && clang -Wno-override-module -c %s -o %s && clang -Wno-override-module -c %s -o %s && clang -Wno-override-module -c %s -o %s && nasm -f elf64 runtime/runtime.asm -o %s && nasm -f elf64 runtime/start.asm -o %s && ld -o %s %s %s %s %s %s %s && %s > %s",
        inputLl, inputObj,
        stringLl, stringObj,
        memoryLl, memoryObj,
        ioLl, ioObj,
        runtimeObj,
        startObj,
        outputExe,
        startObj,
        runtimeObj,
        stringObj,
        memoryObj,
        ioObj,
        inputObj,
        outputExe,
        stdoutPath
    );

    if (!runCommand(command)) {
        return NULL;
    }

    file = fopen(stdoutPath, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    captured = malloc((size_t) size + 1);
    if (captured != NULL) {
        size_t bytes = fread(captured, 1, (size_t) size, file);
        captured[bytes] = '\0';
    }

    fclose(file);
    removeIfExists(sourcePath);
    removeIfExists(inputLl);
    removeIfExists(inputObj);
    removeIfExists(stringLl);
    removeIfExists(stringObj);
    removeIfExists(memoryLl);
    removeIfExists(memoryObj);
    removeIfExists(ioLl);
    removeIfExists(ioObj);
    removeIfExists(runtimeObj);
    removeIfExists(startObj);
    removeIfExists(outputExe);
    removeIfExists(stdoutPath);
    rmdir(dirTemplate);

    return captured;
}
