 #include "test_support.h"

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
