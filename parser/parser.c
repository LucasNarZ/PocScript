#include "parser.h"

Node *createNode(char *value, char *description){
    Node *node = (Node*)(malloc(sizeof(Node)));
    node->children = NULL;
    node->parent = NULL;
    node->numChildren = 0;
    node->value = value;
    node->description = description;
    return node;
}

Node *allocNode(Node *base, Node *newNode){
    newNode->parent = base;
    base->numChildren++;
    base->children = (Node**)(realloc(base->children, base->numChildren * sizeof(Node*)));
    base->children[base->numChildren - 1] = newNode;  
    return base;
}

void callAnteriorAndNext(Node *newNode, Token *token){
    addNodes(newNode, token->anterior);
    if(token->next != NULL){
        addNodes(newNode, token->next);
    }
}

void addNodes(Node *base, Token *token){
    printf("%s\n", token->value);
    if(strcmp(token->value, "if") == 0){
        Node *newNode = createNode("if", "statment");
        addNodes(newNode, token->next);
    }else if(strcmp(token->type, "ASSIGN") == 0 || strcmp(token->type, "OPERATOR") == 0){
        Node *newNode = createNode(token->value, token->type);
        allocNode(base, newNode);
        callAnteriorAndNext(newNode, token);
    }else if((strcmp(token->type, "IDENTIFIER") == 0 || strcmp(token->type, "INTEGER") == 0) && strcmp(base->value, "ROOT") != 0 && (token->anterior == NULL || strcmp(base->description, token->anterior->type) != 0 || strcmp(token->next->type, "DELIMITER") == 0)){
        Node *newNode = createNode(token->value, token->type);
        allocNode(base, newNode);
        printf("description: %s\n", base->description);
        if(token->next != NULL && (strcmp(base->value, "ROOT") == 0|| strcmp(token->next->type, "ASSIGN") != 0 && strcmp(token->next->type, "OPERATOR") != 0)){
            addNodes(base, token->next);
        }
    }else if(strcmp(token->value, ";") == 0){
        if(token->next != NULL){
            Node *temp = (Node*)(malloc(sizeof(Node)));
            temp = base;
            while(temp->parent != NULL){
                if(strcmp(temp->description, "KEY") == 0){
                    addNodes(temp, token->next);
                    break;
                }
                temp = temp->parent;
            }
            addNodes(root, token->next);
        }
    }else{
        if(token->next != NULL){
            addNodes(base, token->next);
        }
    }
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

