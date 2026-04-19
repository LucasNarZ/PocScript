#include "parser.h"
#include "../utils/utils.h"
#include "../lexer/lexer.h"

static bool isTypeToken(const Token *token){
    if(token == NULL){
        return false;
    }

    return token->type == TOKEN_TYPE_ARRAY || token->type == TOKEN_TYPE_INT || token->type == TOKEN_TYPE_FLOAT || token->type == TOKEN_TYPE_CHAR || token->type == TOKEN_TYPE_BOOL;
}

static bool isTokenType(const Token *token, TokenType type){
    return token != NULL && token->type == type;
}

static bool isElseIfSequence(const Token *token){
    return isTokenType(token, TOKEN_KW_ELSE) && token->next != NULL && token->next->type == TOKEN_KW_IF;
}

Node *parseFactor(Token **token){
    if(*token == NULL) return NULL;
    
    if((*token) != NULL && ((*token)->type == TOKEN_NUMBER || (*token)->type == TOKEN_STRING || (*token)->type == TOKEN_BOOL)){
        Node *node = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
        *token = (*token)->next;
        return node;
    }

    if((*token) != NULL && ((*token)->type == TOKEN_IDENTIFIER)){
        Node *node = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
        bool isDeclaration = (*token)->next != NULL && (*token)->next->type == TOKEN_DOUBLE_COLON && (*token)->next->next != NULL && isTypeToken((*token)->next->next);

        if(isDeclaration){
            *token = (*token)->next->next;
            Node *type = NULL;
            if((*token)->type == TOKEN_TYPE_ARRAY){
                type = parseType(node, token);
            }else{
                type = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
                node = allocNode(node, type);
            }
        }else if(isTokenType((*token)->next, TOKEN_LPAREN)){
            Node *args = createNode("CALL_ARGS", "ARGS");
            allocNode(node, args);
            *token = (*token)->next->next;
            parseCallArguments(args, token);
        }else if(isTokenType((*token)->next, TOKEN_LBRACKET)){
            Node *indices = createNode("ARRAY_INDICES", "ARRAY_INDICES");
            allocNode(node, indices);
            (*token) = (*token)->next->next;
            parseArrayIndices(indices, token);
        }
        *token = (*token)->next;
        return node;
    }

    if(isTokenType(*token, TOKEN_LPAREN)){
        *token = (*token)->next;
        Node *node = parseLogical(token);
        if(!isTokenType(*token, TOKEN_RPAREN)){
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
    if(isTokenType(*token, TOKEN_BANG)){
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

    while((*token) != NULL && ((*token)->type == TOKEN_STAR || (*token)->type == TOKEN_SLASH)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseNegation(token));
        
        node = opNode;
    }
    return node;
}

Node *parseExpression(Token **token){
    Node *node = parseTerm(token);

    while((*token) != NULL && ((*token)->type == TOKEN_PLUS || (*token)->type == TOKEN_MINUS)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseTerm(token));
        node = opNode;
    }
    return node;
}

Node *parseComparison(Token **token){
    Node *node = parseExpression(token);

    while((*token) != NULL && ((*token)->type == TOKEN_GT || (*token)->type == TOKEN_LT || (*token)->type == TOKEN_GT_EQ || (*token)->type == TOKEN_LT_EQ || (*token)->type == TOKEN_EQ_EQ || (*token)->type == TOKEN_NOT_EQ)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseExpression(token));
        node = opNode;
    }
    return node;
}

Node *parseLogical(Token **token){
    Node *node = parseComparison(token);

    while((*token) != NULL && ((*token)->type == TOKEN_AND_AND || (*token)->type == TOKEN_OR_OR)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        opNode = allocNode(opNode, parseComparison(token));
        node = opNode;
    }

    return node;
}

Node *parseAssign(Token **token){
    Node *node = parseLogical(token);

    if((*token) != NULL && ((*token)->type == TOKEN_ASSIGN || (*token)->type == TOKEN_PLUS_ASSIGN || (*token)->type == TOKEN_MINUS_ASSIGN)){
        Node *opNode = createNode((*token)->value, "OPERATOR");
        *token = (*token)->next;
        
        opNode = allocNode(opNode, node);
        if(!isTokenType(*token, TOKEN_LBRACE)){
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
    if(isTokenType(*token, TOKEN_KW_RET)){
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

    if(isTokenType(*token, TOKEN_SEMICOLON)){
        *token = (*token)->next;
        allocNode(root, node);
        parseBlock(root, token);
    } 

    return root;
}

Node *parseBlock(Node *root, Token **token){
    if((*token) != NULL && (isTokenType(*token, TOKEN_KW_IF) || isElseIfSequence(*token))){
        Node *opNode;
        if(isElseIfSequence(*token)){
            Node *opNode2 = createNode("else", "BLOCK");
            opNode = createNode("if", "SUB-BLOCK");
            allocNode(root, opNode2);
            allocNode(opNode2, opNode);
        }else{
            opNode = createNode((*token)->value, "BLOCK");
            allocNode(root, opNode);
        }
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseLogical(token));
        *token = (*token)->next->next;
        parseBlock(opNode, token);
        if((*token)->next != NULL){
            *token = (*token)->next;
            if(isElseIfSequence(*token)){
                parseBlock(opNode, token);
            }else if(isTokenType(*token, TOKEN_KW_ELSE)){
                Node *elseNode = createNode((*token)->value, "BLOCK");
                *token = (*token)->next->next;
                allocNode(opNode, parseBlock(elseNode, token));
                if((*token)->next != NULL){
                    *token = (*token)->next;
                }
            }
            if(strcmp(opNode->value, "if") == 0 && strcmp(opNode->description, "SUB-BLOCK") != 0){
                parseBlock(root, token);
            }
        }
    }else if(isTokenType(*token, TOKEN_KW_WHILE)){
        Node *opNode = createNode((*token)->value, "BLOCK");
        *token = (*token)->next->next;
        opNode = allocNode(opNode, parseLogical(token));
        *token = (*token)->next->next;
        allocNode(root, parseBlock(opNode, token));
        if((*token)->next != NULL){
            *token = (*token)->next;
            parseBlock(root, token);
        }
    }else if(isTokenType(*token, TOKEN_KW_FOR)){
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
    }else if(isTokenType(*token, TOKEN_KW_FUNC)){
        *token = (*token)->next;
        Node *func = createNode("FUNCTION", "FUNCTION");
        Node *opNode = createNode((*token)->value, "DECLARATION");
        Node *args = createNode("ARGS", "ARGS");
        allocNode(opNode, args);
        *token = (*token)->next->next;
        parseArguments(args, token);
        *token = (*token)->next->next;
        allocNode(func, args);

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

void parseArguments(Node *func, Token **token){
    if(isTokenType(*token, TOKEN_LPAREN)){
        *token = (*token)->next;
    }
    while(!isTokenType(*token, TOKEN_RPAREN)){
        allocNode(func, parseFactor(token));
        if(isTokenType(*token, TOKEN_COMMA)){
            *token = (*token)->next;
        }
    };
}

Node *parseType(Node *var, Token **token){
    Node *currentNode = var;
    Node *type = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
    Node *initialType = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
    allocNode(currentNode, type);
    currentNode = type;
    (*token) = (*token)->next;
    while(strcmp((*token)->value, "<") == 0){
        (*token) = (*token)->next;
        type = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
        allocNode(currentNode, type);
        currentNode = type;
        (*token) = (*token)->next;
    }

    while(strcmp((*token)->value, ">") == 0){
        (*token) = (*token)->next;
    }
    while(isTokenType(*token, TOKEN_LBRACKET)){
        (*token) = (*token)->next;
        Node *size = createNode((*token)->value, "LIMIT");
        allocNode(var, size);
        (*token) = (*token)->next->next;
    }
    (*token) = (*token)->anterior;
    return initialType;
}

void parseVector(Node *node, Token **token){
    while((*token)->next != NULL && !isTokenType(*token, TOKEN_RBRACE) && !isTokenType(*token, TOKEN_SEMICOLON)){
        Node *element = createNode((*token)->value, (char *)tokenTypeName((*token)->type));
        allocNode(node, element);
        (*token) = (*token)->next;
        if(isTokenType(*token, TOKEN_COMMA)){
            (*token) = (*token)->next;
        }
    }
}

void parseArray(Node *node, Token **token){
    if((*token)->next != NULL && isTokenType(*token, TOKEN_LBRACE)){
        Node *array = createNode("ARRAY_LITERAL", "ARRAY_LITERAL");
        allocNode(node, array);
        (*token) = (*token)->next;
        parseArray(array, token);
        while((*token)->next != NULL && !isTokenType(*token, TOKEN_SEMICOLON)){
            
            if(isTokenType((*token)->next, TOKEN_SEMICOLON)){
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
    while(!isTokenType(*token, TOKEN_RPAREN)){
        allocNode(func, parseLogical(token));
        if(isTokenType(*token, TOKEN_COMMA)){
            *token = (*token)->next;
        }
    };
}

void parseArrayIndices(Node *array, Token **token){
    while((*token)->type == TOKEN_NUMBER || (*token)->type == TOKEN_IDENTIFIER){
        Node *index = createNode((*token)->value, "LIMIT");
        allocNode(array, index);
        (*token) = (*token)->next;
        if((*token) != NULL && (*token)->next != NULL && isTokenType((*token)->next, TOKEN_LBRACKET)){
            (*token) = (*token)->next->next;
        }
    }
}
