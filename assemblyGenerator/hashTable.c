#include "hashTable.h"
#include <stdlib.h>
#include <string.h>
#include "../constants.h"

Variable hashTable[HASH_TABLE_SIZE];

int createHash(char *key, int size){
    int sum = 0;
    for(int i = 0; i < size;i++){
        sum += (unsigned char)key[i];
    }
    return sum;
}

Pair *createPair(Pair *hashTable[HASH_TABLE_SIZE], char *key, int size, char *variable){
    int hash = createHash(key, size);
    Pair *pair = (Pair *)malloc(sizeof(Pair));
    strcpy(pair->key, key);
    strcpy(pair->value, variable);
    pair->next = hashTable[hash];
    hashTable[hash] = pair;
    return variable;
}

Variable *getValue(Pair *hashTable[HASH_TABLE_SIZE], char *key, int size){
    int hash = createHash(key, size);
    Pair *current = hashTable[hash];
    while(current != NULL){
        if(strcmp(current->key, key) == 0){
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}