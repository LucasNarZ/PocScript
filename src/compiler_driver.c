#include "compiler_driver.h"

#include "ast.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

#include <stdio.h>

static bool compileTokensToLlvmIr(Token *tokens, const char *output_path) {
    AstNode *root;
    Parser parser;
    SemanticResult semantic;
    IRModule *module = NULL;
    size_t i;
    bool ok = false;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);
    semantic = semanticAnalyze(root);

    if (semantic.errors.count > 0) {
        printf("\n");
        for (i = 0; i < semantic.errors.count; i++) {
            SemanticError *error = &semantic.errors.items[i];

            if (error->line > 0 && error->column > 0) {
                printf("%s at line %d, column %d: %s\n",
                       semanticErrorKindName(error->kind), error->line, error->column,
                       error->message);
            } else {
                printf("%s: %s\n", semanticErrorKindName(error->kind), error->message);
            }
        }

        goto cleanup;
    }

    module = irBuildModule(root, &semantic);
    ok = module != NULL && irPrintModuleToFile(module, output_path);
    if (!ok) {
        remove(output_path);
        fprintf(stderr, "IR generation failed\n");
    }

cleanup:
    irModuleFree(module);
    semanticResultFree(&semantic);
    astFree(root);
    return ok;
}

bool compileSourceFileToLlvmIr(const char *input_path, const char *output_path) {
    Token *tokens = tokenizeFile(input_path);
    bool ok;

    if (tokens == NULL) {
        fprintf(stderr, "Error opening %s\n", input_path);
        return false;
    }

    ok = compileTokensToLlvmIr(tokens, output_path);
    freeTokens(tokens);
    return ok;
}

bool compileSourceStringToLlvmIr(const char *input, const char *output_path) {
    Token *tokens = tokenizeString(input);
    bool ok;

    if (tokens == NULL) {
        fprintf(stderr, "Error tokenizing source string\n");
        return false;
    }

    ok = compileTokensToLlvmIr(tokens, output_path);
    freeTokens(tokens);
    return ok;
}
