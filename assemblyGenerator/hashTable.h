#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../types.h"

extern hashTable;

int createHash(char *key, int size);
Pair *createPair(Pair *hashTable[HASH_TABLE_SIZE], char *key, int size, char *variable);
Variable *getValue(Pair *hashTable[HASH_TABLE_SIZE], char *key, int size);

#endif