#include "test_support.h"

#include "compiler_driver.h"

AstNode *parseRootFromString(const char *input) {
    Token *tokens = tokenizeString(input);
    Parser parser;
    AstNode *root;

    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);

    freeTokens(tokens);
    return root;
}

char *nodeTreeToString(AstNode *root) {
    return astToString(root);
}

SemanticResult analyzeRootFromString(const char *input) {
    AstNode *root = parseRootFromString(input);
    SemanticResult result = semanticAnalyze(root);

    astFree(root);
    return result;
}

IRModule *buildIrModuleFromString(const char *input) {
    AstNode *root = parseRootFromString(input);
    SemanticResult semantic = semanticAnalyze(root);
    IRModule *module = NULL;

    if (semantic.errors.count == 0) {
        module = irBuildModule(root, &semantic);
    }

    semanticResultFree(&semantic);
    astFree(root);
    return module;
}

char *emitLlvmIrFromString(const char *input) {
    IRModule *module = buildIrModuleFromString(input);
    char *text = NULL;

    if (module != NULL) {
        text = irPrintModuleToString(module);
        irModuleFree(module);
    }

    return text;
}

bool writeLlvmIrFromStringToFile(const char *input, const char *output_path) {
    return compileSourceStringToLlvmIr(input, output_path);
}
