#include "hashTable.h"
#include <stdlib.h>
#include <string.h>
#include "../constants.h"

Pair *hashTable[HASH_TABLE_SIZE] = {0};

int createHash(char *key){
    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    hash = hash % HASH_TABLE_SIZE;
    return hash;
}

Pair *createPair(Pair *hashTable[HASH_TABLE_SIZE], char *key, char *value){
    int hash = createHash(key);
    Pair *pair = (Pair *)malloc(sizeof(Pair));
    pair->key = (char *)malloc(MAX_VAR_NAME);
    pair->value = (char *)malloc(MAX_VAR_NAME);
    strcpy(pair->key, key);
    strcpy(pair->value, value);
    pair->next = hashTable[hash];
    hashTable[hash] = pair;
    return value;
}

Variable *getValue(Pair *hashTable[HASH_TABLE_SIZE], char *key){
    int hash = createHash(key);
    Pair *current = hashTable[hash];
    while(current != NULL){
        if(strcmp(current->key, key) == 0){
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}