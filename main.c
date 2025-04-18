#include <stdio.h>
#include "./lexer/lexer.h"
#include "./parser/parser.h"
#include "./utils/utils.h"

Stack stack = {0};
ScopeStack scopesStack;


int main(){
    FILE *file_ptr = fopen("input.ps", "r");
    char file[100];
    while(fgets(file, 100, file_ptr)){
        char *input = file;
        getTokens(&head, &input);
    }
    print(head);
    
    Node *root = createNode("ROOT", "ROOT");
    scopesStack.scope[0] = 0;
    parseBlock(root, &head);
    printTreePretty(root);
    return 0;
}