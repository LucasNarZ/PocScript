#include "utils.h"

void popVaribles(Stack *stack, int value){
    for(int i = 0;i < value;i++){
        stack->size--;
    }
}

bool arrayContains(char *array[], int size, const char *value){
    for (int i = 0; i < size; i++) {
        if (strcmp(array[i], value) == 0) {
            return true;
        }
    }
    return false;
}

void printTreeWithBranches(Node *node, int depth, int is_last[]){
    if (node == NULL) return;

    for (int i = 0; i < depth - 1; i++){
        if (is_last[i])
            printf("   ");
        else
            printf("  |");
    }

    if (depth > 0) printf("  ├─");

    if (node->value) printf(" (%s)", node->value);
    printf("\n");


    for (int i = 0; i < node->numChildren; i++){
        is_last[depth] = (i == node->numChildren - 1);
        printTreeWithBranches(node->children[i], depth + 1, is_last);
    }
}

void printTreePretty(Node *root){
    int is_last[100] = {0};  
    printTreeWithBranches(root, 0, is_last);
}

void printType(Node *typeNode){
    if (typeNode == NULL) {
        printf("Unknown");
        return;
    }
    if (typeNode->numChildren > 0) {
        printf("%s[", typeNode->value); 
        for (int i = 0; i < typeNode->numChildren; i++) {
            if (i > 0) printf(", ");
            printType(typeNode->children[i]);
        }
        printf("]"); 
    } else {
        printf("%s", typeNode->value);
    }
}

void printStack(Stack *stack) {
    printf("\n====== Stack ======\n");
    if (stack->size == 0) {
        printf("|   (empty)       |\n");
    } else {
        for(int i = stack->size - 1; i >= 0; i--){
            Variable *var = stack->variables[i]; 
            Node *typeNode = var->type; 
            

            if(i == stack->size - 1){
                printf("| %2d: %-12s ", i, var->name);
                printType(typeNode); 
                printf(" <--- top\n");
            }else{
                printf("| %2d: %-12s ", i, var->name);
                printType(typeNode);
                printf(" |\n");
            }
        }
    }
    printf("===================\n\n");
}

void printScopeStack(ScopeStack *stack){
    printf("\n=== Scope Stack ===\n");
    for(int i = stack->size; i >= 0; i--){
        if(i == stack->size - 1){
            printf("| %2d: %d <--- top\n", i, stack->scope[i]);
        }else{
            printf("| %2d: %d           |\n", i, stack->scope[i]);
        }
    }
    
    printf("===================\n\n");
}