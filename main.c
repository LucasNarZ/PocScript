#include <stdio.h>
#include "./lexer/lexer.h"
#include "./parser/parser.h"

Stack stack = {0};
ScopeStack scopesStack;


int main(){
    FILE *file_ptr = fopen("input.txt", "r");
    char file[100];
    while(fgets(file, 100, file_ptr)){
        char *input = file;
        getTokens(&head, &input);
    }
    Node *root = createNode("ROOT", "ROOT");
    scopesStack.scope = malloc(MAX_SIZE_SCOPE * sizeof(int));;
    scopesStack.scope[0] = 0;
    parseBlock(root, &head);
    printTreePretty(root);
    return 0;
}