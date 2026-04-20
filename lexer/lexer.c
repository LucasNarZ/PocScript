#include "lexer.h"

Token *head = NULL;

static void appendToken(Token **head, TokenType type, const char *value, int line, int column) {
    Token *token = malloc(sizeof(Token));

    token->type = type;
    token->value = malloc(strlen(value) + 1);
    strcpy(token->value, value);
    token->line = line;
    token->column = column;
    token->anterior = NULL;
    token->next = NULL;

    if (*head == NULL) {
        *head = token;
        return;
    }

    {
        Token *temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = token;
        token->anterior = temp;
    }
}

TokenDefinition types[NUM_TYPES] = {
    {TOKEN_NUMBER, "^([0-9]*\\.[0-9]+|[0-9]+)"},
    {TOKEN_BOOL, "^(true|false)\\b"},
    {TOKEN_COMMENT, "//.*"},
    {TOKEN_TYPE_ARRAY, "^(Array)\\b"},
    {TOKEN_TYPE_INT, "^(int)\\b"},
    {TOKEN_TYPE_FLOAT, "^(float)\\b"},
    {TOKEN_TYPE_CHAR, "^(char)\\b"},
    {TOKEN_TYPE_BOOL, "^(bool)\\b"},
    {TOKEN_KW_IF, "^(if)\\b"},
    {TOKEN_KW_ELSE, "^(else)\\b"},
    {TOKEN_KW_FOR, "^(for)\\b"},
    {TOKEN_KW_WHILE, "^(while)\\b"},
    {TOKEN_KW_FUNC, "^(func)\\b"},
    {TOKEN_KW_RET, "^(ret)\\b"},
    {TOKEN_STRING, "^\"([^\"\\\\]|\\\\.)*\"|^'([^'\\\\]|\\\\.)*'"},
    {TOKEN_EQ_EQ, "^(==)"},
    {TOKEN_GT_EQ, "^(>=)"},
    {TOKEN_LT_EQ, "^(<=)"},
    {TOKEN_NOT_EQ, "^(!=)"},
    {TOKEN_GT, "^(>)"},
    {TOKEN_LT, "^(<)"},
    {TOKEN_BANG, "^(!)"},
    {TOKEN_PLUS_ASSIGN, "^(\\+=)"},
    {TOKEN_MINUS_ASSIGN, "^(-=)"},
    {TOKEN_ASSIGN, "^(=)"},
    {TOKEN_DOUBLE_COLON, "^(::)"},
    {TOKEN_IDENTIFIER, "^[_a-zA-Z][_a-zA-Z0-9]*"},
    {TOKEN_OR_OR, "^(\\|\\|)"},
    {TOKEN_AND_AND, "^(&&)"},
    {TOKEN_PLUS, "^(\\+)"},
    {TOKEN_MINUS, "^(-)"},
    {TOKEN_STAR, "^(\\*)"},
    {TOKEN_SLASH, "^(\\/)"},
    {TOKEN_LPAREN, "^(\\()"},
    {TOKEN_RPAREN, "^(\\))"},
    {TOKEN_LBRACE, "^(\\{)"},
    {TOKEN_RBRACE, "^(\\})"},
    {TOKEN_LBRACKET, "^(\\[)"},
    {TOKEN_RBRACKET, "^(\\])"},
    {TOKEN_SEMICOLON, "^(;)"},
    {TOKEN_COMMA, "^(,)"},
    {TOKEN_WHITESPACE, "^[ \t\n]+"}
};

const char *tokenTypeName(TokenType type){
    switch(type){
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_TYPE_ARRAY: return "TYPE_ARRAY";
        case TOKEN_TYPE_INT: return "TYPE_INT";
        case TOKEN_TYPE_FLOAT: return "TYPE_FLOAT";
        case TOKEN_TYPE_CHAR: return "TYPE_CHAR";
        case TOKEN_TYPE_BOOL: return "TYPE_BOOL";
        case TOKEN_KW_IF: return "KW_IF";
        case TOKEN_KW_ELSE: return "KW_ELSE";
        case TOKEN_KW_FOR: return "KW_FOR";
        case TOKEN_KW_WHILE: return "KW_WHILE";
        case TOKEN_KW_FUNC: return "KW_FUNC";
        case TOKEN_KW_RET: return "KW_RET";
        case TOKEN_STRING: return "STRING";
        case TOKEN_EQ_EQ: return "EQ_EQ";
        case TOKEN_GT_EQ: return "GT_EQ";
        case TOKEN_LT_EQ: return "LT_EQ";
        case TOKEN_NOT_EQ: return "NOT_EQ";
        case TOKEN_GT: return "GT";
        case TOKEN_LT: return "LT";
        case TOKEN_BANG: return "BANG";
        case TOKEN_PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_DOUBLE_COLON: return "DOUBLE_COLON";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_OR_OR: return "OR_OR";
        case TOKEN_AND_AND: return "AND_AND";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_EOF: return "EOF";
        case TOKEN_WHITESPACE: return "WHITESPACE";
    }

    return "UNKNOWN";
}


void getTokens(Token **head, char **input, int *line, int *column){
    regex_t regex;
    regmatch_t match;
    bool matched = false;
    while(**input != '\0'){
        matched = false;
        for(int i = 0;i < NUM_TYPES;i++){
            if (regcomp(&regex, types[i].pattern, REG_EXTENDED) != 0) {
                char errbuf[256];
                regerror(1, &regex, errbuf, sizeof(errbuf));
                fprintf(stderr, "Regex compilation failed for type %s: %s\n", tokenTypeName(types[i].type), errbuf);
                exit(1);
            }
            if(regexec(&regex, *input, 1, &match, 0) == 0){
                int length = match.rm_eo - match.rm_so;
                if(types[i].type != TOKEN_WHITESPACE && types[i].type != TOKEN_COMMENT){
                    char *value = malloc(length + 1);

                    strncpy(value, *input, length);
                    value[length] = '\0';
                    appendToken(head, types[i].type, value, *line, *column);
                    free(value);
                }

                for(int j = 0; j < length; j++){
                    if((*input)[j] == '\n'){
                        (*line)++;
                        *column = 1;
                    }else{
                        (*column)++;
                    }
                }

                *input += length;
                matched = true;
                regfree(&regex);
                break;
            }
            regfree(&regex);
        }
        if (!matched) {
            fprintf(stderr, "Unrecognized token: %.10s\n", *input);
            exit(1);
        }
    }

    appendToken(head, TOKEN_EOF, "", *line, *column);

}

Token *tokenizeString(const char *input){
    Token *tokens = NULL;
    char *buffer = malloc(strlen(input) + 1);
    char *cursor = buffer;
    int line = 1;
    int column = 1;

    strcpy(buffer, input);
    getTokens(&tokens, &cursor, &line, &column);

    free(buffer);
    return tokens;
}

Token *tokenizeFile(const char *path) {
    char *input = readFileToString(path);
    Token *tokens;

    if (input == NULL) {
        return NULL;
    }

    tokens = tokenizeString(input);
    free(input);
    return tokens;
}

char *readFileToString(const char *path){
    FILE *file = fopen(path, "r");
    long size;
    char *buffer;

    if(file == NULL){
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    buffer = malloc(size + 1);
    if(buffer == NULL){
        fclose(file);
        return NULL;
    }

    if(fread(buffer, 1, size, file) != (size_t)size){
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

char *tokensToString(Token *head){
    size_t size = 1;
    char *buffer = malloc(size);
    Token *current = head;

    if(buffer == NULL){
        return NULL;
    }

    buffer[0] = '\0';

    while(current != NULL){
        if (current->type == TOKEN_EOF) {
            current = current->next;
            continue;
        }

        size_t lineLength = strlen(tokenTypeName(current->type)) + strlen(current->value) + 2;
        char *nextBuffer = realloc(buffer, size + lineLength);

        if(nextBuffer == NULL){
            free(buffer);
            return NULL;
        }

        buffer = nextBuffer;
        snprintf(buffer + size - 1, lineLength + 1, "%s %s\n", tokenTypeName(current->type), current->value);
        size += lineLength;
        current = current->next;
    }

    return buffer;
}

char *tokenizeFileToString(const char *path){
    char *input = readFileToString(path);
    Token *tokens;
    char *output;

    if(input == NULL){
        return NULL;
    }

    tokens = tokenizeString(input);
    free(input);

    output = tokensToString(tokens);
    freeTokens(tokens);
    return output;
}

void freeTokens(Token *head){
    Token *current = head;

    while(current != NULL){
        Token *next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
}

void print(Token *head){
    Token *temp = head;
    while(temp != NULL){
        printf("%s\n", tokenTypeName(temp->type));
        printf("%s\n", temp->value);
        printf("%d\n", temp->line);
        printf("%d\n", temp->column);
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
        printf("%s\n", tokenTypeName(temp->type));
        printf("%s\n", temp->value);
        printf("\n");
        temp = temp->anterior;
    }
}
