#include "lexer.h"

Token *head = NULL;

TokenType types[NUM_TYPES] = {
    {"INTEGER", "(^[0-9]+)"},
    {"KEY", "(^(if|else|for|while))"},
    {"OPERATOR", "(^[\+|\-|\*|\/|==])"},
    {"ASSIGN", "(^[=])"},
    {"DELIMITER", "(^[;{}\(\)])"},
    {"IDENTIFIER", "(^[_a-zA-Z][_a-zA-Z0-9]*)"},
    {"WHITESPACE", "(^[ \t\n]+)"}
};

void getTokens(Token **head, char **input){
    regex_t regex;
    regmatch_t match;
    while(**input != '\0'){
        for(int i = 0;i < NUM_TYPES;i++){
            if(regcomp(&regex, types[i].pattern, REG_EXTENDED) != 0){
                printf("Failed to compile regex");
                exit(1);
            }
            if(regexec(&regex, *input, 1, &match, 0) == 0){
                int length = match.rm_eo - match.rm_so;
                if(strcmp(types[i].type, "WHITESPACE") != 0){
                    Token *token = (Token*)(malloc(sizeof(Token)));
                    token->type = types[i].type;
                    token->value = (char*)(malloc(length + 1));
                    strncpy(token->value, *input, length);
                    token->value[length] = '\0'; 
                    token->anterior = NULL;
                    token->next = NULL;

                    if(*head == NULL){
                        *head = token;
                    }else{
                        Token *temp = *head;
                        while(temp->next != NULL){
                            temp = temp->next;
                        }
                        temp->next = token;
                        token->anterior = temp;
                    }
                }
                *input += length;
                regfree(&regex);
            }
        }
    }
}

void print(Token *head){
    Token *temp = head;
    while(temp != NULL){
        printf("%s\n", temp->type);
        printf("%s\n", temp->value);
        printf("\n");
        temp = temp->next;
    }
}

void printInv(Token *head){
    Token *temp = head;
    while(temp->next != NULL){
        temp = temp->next;
    }
    while(temp != NULL){
        printf("%s\n", temp->type);
        printf("%s\n", temp->value);
        printf("\n");
        temp = temp->anterior;
    }
}


