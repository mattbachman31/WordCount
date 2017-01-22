#ifndef STORE_H
#define STORE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dlfcn.h>

struct innerPair{
	char* key;
	int val;
};

typedef struct innerPair Pair;

struct innerMap{
	Pair* arr;
	int capacity;
	int numFilled;
	char* currentPool;
	int currentPoolIndex;
	int currentPoolCapacity;
	char** poolList;
	int poolListSize;
	int poolListCapacity;
};

typedef struct innerMap Map;

#define LOAD_FACTOR 0.75

#define START_CAPACITY 128

#define POOL_ALLOC_SIZE 2500

#define POOL_LIST_SIZE_INIT 8

void initialize(Map* m);

Map* create();

int hash(Map *m, char* orig);

int increment(Map *m, char* word);

int getCount(Map *m, char* word);

void destroy(Map* map);

#endif
