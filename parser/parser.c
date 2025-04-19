#include "parser.h"
#include "../utils/utils.h"

Node *parseFactor(Token **token){
    if(*token == NULL) return NULL;
    
    if((*token) != NULL && (strcmp((*token)->type, "NUMBER") == 0 || strcmp((*token)->type, "STRING") == 0)){
        Node *node = createNode((*token)->value, (*token)->type);
        *token = (*token)->next;
        return node;
    }

    if((*token) != NULL && (strcmp((*token)->type, "IDENTIFIER") == 0)){
        Node *node = createNode((*token)->value, (*token)->type);
        
        if(!(arrayContains(stack.variables, stack.size, (*token)->value))){
            if(strcmp((*token)->next->next->next->type, "TYPE") != 0 && strcmp((*token)->next->next->next->type, "COMPOSED_TYPE") != 0){
                fprintf(stderr, "NameError: Undefined Variable: %s\n", (*token)->value);
                exit(1);
            }
            stack.variables[stack.size++] = (*token)->value;
            scopesStack.scope[scopesStack.size]++;
            
            *token = (*token)->next->next->next;
            if(strcmp((*token)->type, "COMPOSED_TYPE") == 0){
                parseType(node, token);
            }else{
                Node *type = createNode((*token)->value, (*token)->type);
                node = allocNode(node, type);
            }
        }else{
            if(strcmp((*token)->next->next->next->type, "TYPE") == 0){
                fprintf(stderr, "NameError: redeclared Variable: %s\n", (*token)->value);
                exit(1);
            }
        }
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
        if(strcmp((*token)->value, "{") != 0){
            opNode = allocNode(opNode, parseLogical(token));
        }else{
            Node *arrayNode = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
            allocNode(opNode, arrayNode);
            (*token) = (*token)->next;
            parseArray(arrayNode, token);
        }
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
        scopesStack.size++;
        scopesStack.scope[scopesStack.size] = 0;
        Node *opNode = createNode((*token)->value, "BLOCK");
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseLogical(token));
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        if((*token)->next != NULL){
            popVaribles(&stack, scopesStack.scope[scopesStack.size]);
            scopesStack.size--;
            *token = (*token)->next;
            parseBlock(root, token);
        }

    }else if((*token) != NULL && (strcmp((*token)->value, "for") == 0)){
        scopesStack.size++;
        scopesStack.scope[scopesStack.size] = 0;
        
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
            popVaribles(&stack, scopesStack.scope[scopesStack.size]);
            scopesStack.size--;
            *token = (*token)->next;
            parseBlock(root, token);
        }
    }else if((*token) != NULL && (strcmp((*token)->value, "func") == 0)){
        scopesStack.size++;
        scopesStack.scope[scopesStack.size] = 0;
        *token = (*token)->next;
        Node *opNode = createNode((*token)->value, "DECLARATION");
        Node *args = createNode("ARGS", "ARGS");
        allocNode(opNode, args);
        *token = (*token)->next;
        parseArguments(args, token);
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        if((*token)->next != NULL){
            popVaribles(&stack, scopesStack.scope[scopesStack.size]);
            scopesStack.size--;
            *token = (*token)->next;
            parseBlock(root, token);
        }
    }else{
        parseStatement(root, token);
    }
    return root;
}

void *parseArguments(Node *func, Token **token){
    do{
        *token = (*token)->next;
        allocNode(func, parseFactor(token));
    }while(strcmp((*token)->value, ",") == 0);
}

void *parseType(Node *var, Token **token){
    Node *currentNode = var;
    Node *type = createNode((*token)->value, (*token)->type);
    allocNode(currentNode, type);
    currentNode = type;
    (*token) = (*token)->next;
    while(strcmp((*token)->value, "<") == 0){
        (*token) = (*token)->next;
        type = createNode((*token)->value, (*token)->type);
        allocNode(currentNode, type);
        currentNode = type;
        (*token) = (*token)->next;
    }

    while(strcmp((*token)->value, ">") == 0){
        (*token) = (*token)->next;
    }
    while(strcmp((*token)->value, "[") == 0){
        (*token) = (*token)->next;
        Node *size = createNode((*token)->value, "LIMIT");
        allocNode(var, size);
        (*token) = (*token)->next->next;
    }
    (*token) = (*token)->anterior;
}

void *parseArray(Node *node, Token **token){
    if((*token)->next != NULL && strcmp((*token)->value, "{") == 0){
        Node *array = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
        allocNode(node, array);
        (*token) = (*token)->next;
        parseArray(array, token);
        while((*token)->next != NULL && strcmp((*token)->value, ";") != 0){
            array = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
            allocNode(node, array);
            if(strcmp((*token)->next->value, ";") == 0){
                (*token) = (*token)->next;
            }else{
                (*token) = (*token)->next->next;
            }
            
            parseArray(array, token);
        }
    }else{
        while((*token)->next != NULL && strcmp((*token)->type, "DELIMITER") != 0){
            Node *element = createNode((*token)->value, (*token)->type);
            allocNode(node, element);
            (*token) = (*token)->next->next;
        }
    }

}