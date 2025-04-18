#include "ast.h"

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