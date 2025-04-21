#include "utils.h"

char *getVarType(char *var, Stack *stack){
    for(int i = 0;i < stack->size;i++){
        if(strcmp(stack->variables[i]->name, var) == 0){
            return stack->variables[i]->type->value;
        }
    }
    return NULL;
}

void defineVariables(char *name, Node *type, Stack *stack, ScopeStack *scopesStack){
    stack->variables[stack->size++] = createVariable(name, type);
    scopesStack->scope[scopesStack->size]++;
}

void getVarsNames(char **names, Stack *stack){
    for(int i = 0;i < stack->size;i++){
        names[i] = stack->variables[i]->name;
    }
}

Variable *createVariable(char *name, Node *type){
    Variable *var = (Variable *)(malloc(sizeof(Variable)));
    var->name = name;
    var->type = type;
    return var;
}

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

void cleanStack(Stack *stack, ScopeStack *ScopeStack){
    for(int i = stack->size - 1;i > 0;i--){
        free(stack->variables[i]);
    }
    stack->size = 1;
    ScopeStack->size = 0;
    ScopeStack->scope[0] = 1;
}