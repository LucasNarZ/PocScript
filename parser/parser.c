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

    if((*token) != NULL && (strcmp((*token)->value, "+") == 0 || strcmp((*token)->value, "-") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseExpression(token));
        node = opNode;
    }
    return node;
}

Node *parseComparison(Token **token){
    Node *node = parseExpression(token);

    if((*token) != NULL && (strcmp((*token)->value, ">") == 0 || strcmp((*token)->value, "<") == 0 || strcmp((*token)->value, ">=") == 0 || strcmp((*token)->value, "<=") == 0 || strcmp((*token)->value, "==") == 0 || strcmp((*token)->value, "!=") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseComparison(token));
        node = opNode;
    }
    return node;
}

Node *parseLogical(Token **token){
    Node *node = parseComparison(token);

    if((*token) != NULL && (strcmp((*token)->value, "&&") == 0 || strcmp((*token)->value, "||") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseLogical(token));
        node = opNode;
    }
    return node;
}

Node *parseAssign(Token **token){
    Node *node = parseLogical(token);

    if((*token) != NULL && (strcmp((*token)->value, "=") == 0 || strcmp((*token)->value, "+=") == 0)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseAssign(token));
        node = opNode;
    }
    return node;
}

Node *parseStatement(Node *root, Token **token){
    if(*token == NULL) return NULL;

    Node *node = parseAssign(token);

    if((*token) != NULL && (strcmp((*token)->value, ";") == 0)){
        *token = (*token)->next;
        allocNode(root, node);
        parseBlock(root, token);
    } 


    return root;
}

Node *parseBlock(Node *root, Token **token){
    if((*token) != NULL && (strcmp((*token)->value, "if") == 0 || strcmp((*token)->value, "while") == 0)){
        Node *opNode = createNode((*token)->value, "BLOCK");
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseLogical(token));
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        if((*token)->next != NULL){
            *token = (*token)->next;
            parseBlock(root, token);
        }

    }else if((*token) != NULL && (strcmp((*token)->value, "for") == 0)){
        Node *opNode = createNode((*token)->value, "BLOCK");
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseAssign(token));
        *token = (*token)->next;
        opNode = allocNode(opNode, parseAssign(token));
        *token = (*token)->next;
        opNode = allocNode(opNode, parseAssign(token));
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        if((*token)->next != NULL){
            *token = (*token)->next;
            parseBlock(root, token);
        }
    }else{
        parseStatement(root, token);
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

