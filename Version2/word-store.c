#include "word-store.h"

void initialize(Map* m){
	for(int i=0; i < m->capacity; ++i){
		m->arr[i].val = 0;
	}
}

Map* create(){
	Map* ret = (Map *)malloc(sizeof(Map));
	if(!ret){
		fprintf(stderr, "Malloc failed, terminating program.\n");
		exit(1);
	}
	ret->arr = (Pair *)malloc(sizeof(Pair) * START_CAPACITY);
	if(!ret->arr){
		fprintf(stderr, "Malloc failed, terminating program.\n");
		exit(1);
	}
	ret->currentPool = (char *)malloc(sizeof(char) * POOL_ALLOC_SIZE);
	if(!ret->currentPool){
		fprintf(stderr, "Malloc failed, terminating program.\n");
		exit(1);
	}
	ret->poolList = (char **)malloc(sizeof(char *) * POOL_LIST_SIZE_INIT);
	if(!ret->poolList){
		fprintf(stderr, "Malloc failed, terminating program.\n");
		exit(1);
	}
	ret->poolList[0] = ret->currentPool;
	ret->poolListSize = 1;
	ret->poolListCapacity = POOL_LIST_SIZE_INIT;
	ret->currentPoolIndex = 0;
	ret->currentPoolCapacity = POOL_ALLOC_SIZE;
	ret->capacity = START_CAPACITY;
	ret->numFilled = 0;
	initialize(ret);
	return ret;
}

int hash(Map *m, char* orig){
	int sumOfAscii = 0;
	int i = 0;
	char c = orig[i];
	while(c != '\0'){
		sumOfAscii += c;
		++i;
		c = orig[i];
	}
	return sumOfAscii % m->capacity;
}

int insert(Map *m, char* word, int index, int count){
	//Assured it does not exist at this point by increment
	m->arr[index].key = word;
	m->arr[index].val = count;
	++(m->numFilled);

	//Check new Load Factor...Double capacity if above
	float currentLF = (float)m->numFilled / (float)m->capacity;
	if(currentLF > LOAD_FACTOR){
		Pair* newInnerArr = (Pair *)malloc(sizeof(Pair) * m->capacity * 2);
		if(!newInnerArr){
			fprintf(stderr, "Malloc failed, terminating program.\n");
			exit(1);
		}
		Pair* oldBuff = m->arr;
		m->capacity *= 2;
		m->arr = newInnerArr;
		m->numFilled = 0;
		initialize(m);
		for(int i=0; i<(m->capacity/2); ++i){
			if(oldBuff[i].val > 0){
				int newIndex = hash(m, oldBuff[i].key);
				while(m->arr[newIndex].val > 0){
					newIndex = (newIndex + 1) % m->capacity;
				}
				if(strcmp(oldBuff[i].key, word) == 0){
					index = newIndex;
				}
				insert(m, oldBuff[i].key, newIndex, oldBuff[i].val);
			}
		}
		free(oldBuff);
	}
	return index; //unused info for now
}

int increment(Map *m, char* word, int toInc){
	int index = hash(m, word);
	while(m->arr[index].val > 0){
		if(strcmp(m->arr[index].key, word) == 0){
			m->arr[index].val += toInc;
			return 1;
		}
		index = (index + 1) % m->capacity;
	}
	insert(m, word, index, toInc);
	return 0;
}

int getCount(Map *m, char* word){
	int index = hash(m, word);
	while(m->arr[index].val > 0){
		if(strcmp(m->arr[index].key, word) == 0){
			return m->arr[index].val;
		}
		index = (index + 1) % m->capacity;
	}
	return -1;
}

void destroy(Map* map){
	free(map->arr);
	for(int i = 0; i < map->poolListSize; ++i){
		free(map->poolList[i]);
	}
	free(map->poolList);
	free(map);
}