#include <stdio.h>
#include "./lexer/lexer.h"
#include "./parser/parser.h"
#include "./utils/utils.h"
#include "./assemblyGenerator/generator.h"

Stack stack = {0};
ScopeStack scopesStack;
LineIndices *functionLineIndices;
LineIndices *lineIndices;

int main(){
    FILE *file_ptr = fopen("input.ps", "r");
    char file[100];
    while(fgets(file, 100, file_ptr)){
        char *input = file;
        getTokens(&head, &input);
    }
    
    Node *root = createNode("ROOT", "ROOT");
    Variable *print = (Variable *)malloc(sizeof(Variable));
    Node *type = (Node *)malloc(sizeof(Node));
    print->name = "printInt";
    print->type = type;
    print->type->value = "FUNCTION";
    print->type->description = "FUNCTION";
    print->type->numChildren = 0;
    stack.variables[0] = print;
    stack.size++;
    Variable *printString = (Variable *)malloc(sizeof(Variable));
    Node *type2 = (Node *)malloc(sizeof(Node));
    printString->name = "printString";
    printString->type = type2;
    printString->type->value = "FUNCTION";
    printString->type->description = "FUNCTION";
    printString->type->numChildren = 0;
    stack.variables[1] = printString;
    stack.size++;
    scopesStack.scope[0] = 2;
    parseBlock(root, &head);
    printTreePretty(root);

    cleanStack(&stack, &scopesStack);

    FILE *OUTPUT = NULL;
    lineIndices = malloc(sizeof(LineIndices));
    lineIndices->currentLine = 1;
    lineIndices->globalVariblesLine = 4;

    functionLineIndices = malloc(sizeof(LineIndices));
    functionLineIndices->currentLine = 1;
    functionLineIndices->globalVariblesLine = 4;

    generateAssembly(root, OUTPUT, lineIndices);
    cleanStack(&stack, &scopesStack);
    return 0;
}