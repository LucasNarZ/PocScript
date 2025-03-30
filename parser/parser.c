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



Node *parseFactor(Token **token){
    if(*token == NULL) return NULL;
    
    if((*token) != NULL && (strcmp((*token)->type, "INTEGER") == 0 || strcmp((*token)->type, "IDENTIFIER") == 0)){
        Node *node = createNode((*token)->value, (*token)->type);
        *token = (*token)->next;
        return node;
    }

    return NULL;
}

Node *parseTerm(Token **token){
    Node *node = parseFactor(token);

    if((*token) != NULL && (strcmp((*token)->value, "*") == 0 || strcmp((*token)->value, "/") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseTerm(token));
        node = opNode;
    }
    return node;
}

Node *parseExpression(Token **token){
    Node *node = parseTerm(token);

    if((*token) != NULL && (strcmp((*token)->value, "+") == 0 || strcmp((*token)->value, "-") == 0 || strcmp((*token)->value, "=") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        
        parseExpression(token);
        node = opNode;
    }
    return node;
}

Node *parseStatement(Token **token){
    if(*token == NULL) return NULL;

    Node *node = parseExpression(token);
    root = allocNode(root, node);
    
    if((*token) != NULL && strcmp((*token)->value, ";") == 0){
        Node *root = createNode("ROOT", "ROOT");
        *token = (*token)->next;
        Node *node = parseStatement(token);
        root = allocNode(root, node);
    }
    return root;
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

