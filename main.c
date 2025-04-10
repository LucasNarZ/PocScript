#include <stdio.h>
#include "./lexer/lexer.h"
#include "./parser/parser.h"


int main(){
    FILE *file_ptr = fopen("input.txt", "r");
    char file[100];
    while(fgets(file, 100, file_ptr)){
        char *input = file;
        getTokens(&head, &input);
    }
    Node *root = createNode("ROOT", "ROOT");
    parseStatement(root, &head);
    printTreePretty(root);
    return 0;
}