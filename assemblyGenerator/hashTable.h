#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../types.h"

extern Pair *hashTable[HASH_TABLE_SIZE];

int createHash(char *key);
Pair *createPair(Pair *hashTable[HASH_TABLE_SIZE], char *key, char *variable);
Variable *getValue(Pair *hashTable[HASH_TABLE_SIZE], char *key);

#endif