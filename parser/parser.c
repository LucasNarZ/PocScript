#include "parser.h"
#include "../utils/utils.h"


char *getVarType(char *var, Stack *stack){
    for(int i = 0;i < stack->size;i++){
        if(strcmp(stack->variables[i]->name, var) == 0){
            return stack->variables[i]->type->value;
        }
    }
    return NULL;
}

void defineVariables(char *name, Node *type, Stack *stack){
    stack->variables[stack->size++] = createVariable(name, type);
    scopesStack.scope[scopesStack.size]++;
}

char **getVarsNames(char **names, Stack *stack){
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

Node *parseFactor(Token **token){
    if(*token == NULL) return NULL;
    
    if((*token) != NULL && (strcmp((*token)->type, "NUMBER") == 0 || strcmp((*token)->type, "STRING") == 0 || strcmp((*token)->type, "BOOL") == 0)){
        Node *node = createNode((*token)->value, (*token)->type);
        *token = (*token)->next;
        return node;
    }

    if((*token) != NULL && (strcmp((*token)->type, "IDENTIFIER") == 0)){
        Node *node = createNode((*token)->value, (*token)->type);
        char **names = (char **)(malloc(stack.size * sizeof(char *)));
        getVarsNames(names, &stack);
        if(!(arrayContains(names, stack.size, (*token)->value))){
            if(strcmp((*token)->next->next->next->type, "TYPE") != 0 && strcmp((*token)->next->next->next->type, "COMPOSED_TYPE") != 0){
                fprintf(stderr, "NameError: Undefined Variable: %s\n", (*token)->value);
                exit(1);
            }
            char *name = (*token)->value;
            
            *token = (*token)->next->next->next;
            Node *type = NULL;
            if(strcmp((*token)->type, "COMPOSED_TYPE") == 0){
                type = parseType(node, token);
            }else{
                type = createNode((*token)->value, (*token)->type);
                node = allocNode(node, type);
            }
            defineVariables(name, type, &stack);
        }else{
            if((*token)->next->next->next != NULL && (strcmp((*token)->next->next->next->type, "TYPE") == 0 || strcmp((*token)->next->next->next->type, "COMPOSED_TYPE") == 0)){
                fprintf(stderr, "NameError: redeclared Variable: %s\n", (*token)->value);
                exit(1);
            }

            if(strcmp(getVarType((*token)->value, &stack), "FUNCTION") == 0){
                Node *args = createNode("CALL_ARGS", "ARGS");
                allocNode(node, args);
                *token = (*token)->next->next;
                parseCallArguments(args, token);
                *token = (*token)->anterior;
            }else if(strcmp(getVarType((*token)->value, &stack), "Array") == 0){
                Node *indices = createNode("ARRAY_INDICES", "ARRAY_INDICES");
                allocNode(node, indices);
                (*token) = (*token)->next->next;
                parseArrayIndices(indices, token);
                *token = (*token)->anterior;
            }
        }
        *token = (*token)->next;
        return node;
    }

    if(strcmp((*token)->value, "(") == 0){
        *token = (*token)->next;
        Node *node = parseLogical(token);
        if(strcmp((*token)->value, ")") != 0){
            fprintf(stderr, "SyntaxError: expected ')'\n");
            exit(1);
        }
        *token = (*token)->next;
        return node;
    }

    return NULL;
}

Node *parseNegation(Token **token){
    Node *node = NULL;
    if((*token) != NULL && strcmp((*token)->value, "!") == 0){
        node = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        node = allocNode(node, parseNegation(token));
    }else{
        node = parseFactor(token);

    }
    return node;
}

Node *parseTerm(Token **token){
    Node *node = parseNegation(token);

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

    if((*token) != NULL && (strcmp((*token)->value, "=") == 0 || strcmp((*token)->value, "+=") == 0 || strcmp((*token)->value, "-=") == 0)){
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

Node *parseReturn(Token **token){
    Node *node = NULL;
    if((*token) != NULL && strcmp((*token)->value, "ret") == 0){
        node = createNode((*token)->value, "RETURN");
        *token = (*token)->next;
        node = allocNode(node, parseLogical(token));
    }else{
        node = parseAssign(token);

    }
    return node;
}

Node *parseStatement(Node *root, Token **token){
    if(*token == NULL) return NULL;

    Node *node = parseReturn(token);

    if((*token) != NULL && (strcmp((*token)->value, ";") == 0)){
        *token = (*token)->next;
        allocNode(root, node);
        parseBlock(root, token);
    } 

    return root;
}

Node *parseBlock(Node *root, Token **token){
    if((*token) != NULL && (strcmp((*token)->value, "if") == 0 || strcmp((*token)->value, "else if") == 0)){
        scopesStack.size++;
        scopesStack.scope[scopesStack.size] = 0;
        Node *opNode = createNode((*token)->value, "BLOCK");
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseLogical(token));
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        popVaribles(&stack, scopesStack.scope[scopesStack.size]);
        scopesStack.size--;
        if((*token)->next != NULL){
            *token = (*token)->next;
            if(strcmp((*token)->value, "else if") == 0){
                parseBlock(opNode, token);
            }else if(strcmp((*token)->value, "else") == 0){
                scopesStack.size++;
                scopesStack.scope[scopesStack.size] = 0;
                Node *elseNode = createNode((*token)->value, "BLOCK");
                *token = (*token)->next->next;
                allocNode(opNode, parseBlock(elseNode, token));
                if((*token)->next != NULL){
                    popVaribles(&stack, scopesStack.scope[scopesStack.size]);
                    scopesStack.size--;
                    *token = (*token)->next;
                }
            }
            if(strcmp(opNode->value, "if") == 0){
                parseBlock(root, token);
            }
        }
    }else if((*token) != NULL &&  strcmp((*token)->value, "while") == 0){
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
        *token = (*token)->next;
        char *functionName = (*token)->value;

        Node *func = createNode("FUNCTION", "FUNCTION");
        char **names = (char **)(malloc(stack.size * sizeof(char *)));
        getVarsNames(names, &stack);
        if(arrayContains(names, stack.size, functionName)){
            fprintf(stderr, "NameError: redeclared Variable: %s\n", functionName);
            exit(1);
        }
        defineVariables(functionName, func, &stack);

        scopesStack.size++;
        scopesStack.scope[scopesStack.size] = 0;
        Node *opNode = createNode((*token)->value, "DECLARATION");
        Node *args = createNode("ARGS", "ARGS");
        allocNode(opNode, args);
        *token = (*token)->next->next;
        parseArguments(args, token);
        *token = (*token)->next;
        allocNode(func, args);

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

void parseArguments(Node *func, Token **token){
    if(strcmp((*token)->type, "DELIMITER") == 0){
        *token = (*token)->next;
    }
    while(strcmp((*token)->type, "DELIMITER") != 0){
        allocNode(func, parseFactor(token));
        *token = (*token)->next;
    };
}

Node *parseType(Node *var, Token **token){
    Node *currentNode = var;
    Node *type = createNode((*token)->value, (*token)->type);
    Node *initialType = createNode((*token)->value, (*token)->type);
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
    return initialType;
}

void parseVector(Node *node, Token **token){
    while((*token)->next != NULL && strcmp((*token)->type, "DELIMITER") != 0){
        Node *element = createNode((*token)->value, (*token)->type);
        allocNode(node, element);
        (*token) = (*token)->next->next;
    }
}

void parseArray(Node *node, Token **token){
    if((*token)->next != NULL && strcmp((*token)->value, "{") == 0){
        Node *array = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
        allocNode(node, array);
        (*token) = (*token)->next;
        parseArray(array, token);
        while((*token)->next != NULL && strcmp((*token)->value, ";") != 0){
            
            if(strcmp((*token)->next->value, ";") == 0){
                (*token) = (*token)->next;
            }else{
                array = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
                allocNode(node, array);
                (*token) = (*token)->next->next;
            }
            
            parseArray(array, token);
        }
    }else{
        parseVector(node, token);
    }
}

void parseCallArguments(Node *func, Token **token){
    while(strcmp((*token)->type, "DELIMITER") != 0){
        Node *arg = createNode((*token)->value, (*token)->type);
        allocNode(func, arg);
        *token = (*token)->next->next;
        
    };
}

void parseArrayIndices(Node *array, Token **token){
    while(strcmp((*token)->type, "NUMBER") == 0 || strcmp((*token)->type, "IDENTIFIER") == 0){
        Node *index = createNode((*token)->value, "LIMIT");
        allocNode(array, index);
        (*token) = (*token)->next->next;
        if(strcmp((*token)->value, "[") == 0){
            (*token) = (*token)->next;
        }
    }
}
