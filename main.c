#include <stdio.h>
#include "./lexer/lexer.h"
#include "./parser/parser.h"
#include "./parser/ast.h"

int main(){
    AstNode *root;
    Parser parser;
    Token *tokens = tokenizeFile("input.ps");

    if(tokens == NULL){
        fprintf(stderr, "Error opening input.ps\n");
        return 1;
    }

    print(tokens);
    parserInit(&parser, tokens);
    root = parserParseProgram(&parser);
    astPrintPretty(root);
    astFree(root);
    freeTokens(tokens);

    return 0;
}
