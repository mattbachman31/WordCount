#include "word-count.h"

void updateWithFile(Map* map, char* fileName, Map* noMap, int numTimes){
	FILE* file;
	if(!numTimes){
		file = fopen(fileName, "r");
		if(file == NULL){
			fprintf(stderr, "Could not open file: %s\n", fileName);
			exit(1);
		}
	}
	int bufIndex = map->currentPoolIndex;
	int fileIndex = 0;
	int wordStartFileIndex = 0;
	char c;
	if(!numTimes){c = fgetc(file);}
	else {c = fileName[fileIndex];}
	++fileIndex;
	char inWordBool = 0;
	top: while(c != EOF){
		while(!isalnum(c) && c != '\'' && c != EOF){
			if(!numTimes) {c = fgetc(file);}
			else {c = fileName[fileIndex];}
			++fileIndex;
			++wordStartFileIndex;
		}
		while(isalnum(c) || c == '\''){
			if(map->currentPoolIndex >= map->currentPoolCapacity-1){
				if(bufIndex == 0){
					//current alloc size not big enough for word...
					while(isalnum(c) || c == '\''){
						if(!numTimes) {c = fgetc(file);}
						else {c = fileName[fileIndex];}
						++fileIndex;
					}
					int wordLength = (fileIndex - wordStartFileIndex); //null-terminating included in last ++fileIndex
					char* newBigPool = (char *)malloc(sizeof(char) * wordLength);
					if(!newBigPool){
						fprintf(stderr, "Malloc failed, terminating program.\n");
						exit(1);
					}
					map->poolList[map->poolListSize] = newBigPool;
					map->currentPoolIndex = 0;
					map->currentPoolCapacity = wordLength;
					map->currentPool = newBigPool;
					++map->poolListSize;
					if(map->poolListSize == map->poolListCapacity){
						map->poolList = realloc(map->poolList, (sizeof(char*) * map->poolListCapacity * 2));
						map->poolListCapacity *= 2;
						if(!map->poolList){
							fprintf(stderr, "Malloc failed, terminating program.\n");
							exit(1);
						}
					}
					if(!numTimes){
						fseek(file, wordStartFileIndex, SEEK_SET);
						fileIndex = wordStartFileIndex;
						c = fgetc(file);
					}
					else {
						fileIndex = wordStartFileIndex;
						c = fileName[fileIndex];
					}
					++fileIndex;
					while(isalnum(c) || c == '\''){
						map->currentPool[map->currentPoolIndex] = tolower(c);
						++map->currentPoolIndex;
						if(!numTimes) {c = fgetc(file);}
						else {c = fileName[fileIndex];}
						++fileIndex;
					}
					goto bottom;
				}
				else{
					//allocate new pool
					char* newPool = (char *)malloc(sizeof(char) * POOL_ALLOC_SIZE);
					if(!newPool){
						fprintf(stderr, "Malloc failed, terminating program.\n");
						exit(1);
					}
					map->poolList[map->poolListSize] = newPool;
					map->currentPoolIndex = 0;
					map->currentPool = newPool;
					map->currentPoolCapacity = POOL_ALLOC_SIZE;
					++map->poolListSize;
					bufIndex = 0;
					if(map->poolListSize == map->poolListCapacity){
						map->poolList = realloc(map->poolList, (sizeof(char*) * map->poolListCapacity * 2));
						map->poolListCapacity *= 2;
						if(!map->poolList){
							fprintf(stderr, "Malloc failed, terminating program.\n");
							exit(1);
						}
					}
					if(!numTimes) {
						fseek(file, wordStartFileIndex, SEEK_SET);
						fileIndex = wordStartFileIndex;
						c = fgetc(file);
					}
					else{
						fileIndex = wordStartFileIndex;
						c = fileName[fileIndex];
					}
					++fileIndex;
					goto top;
				}
			}
			inWordBool = 1;
			map->currentPool[map->currentPoolIndex] = tolower(c);
			++map->currentPoolIndex;

			if(!numTimes) {c = fgetc(file);}
			else {c = fileName[fileIndex];}
			++fileIndex;
		}
		bottom: if(inWordBool){
			map->currentPool[map->currentPoolIndex] = '\0';
			++map->currentPoolIndex;
			
			int wasInAlready;
			if(noMap == NULL || getCount(noMap, &map->currentPool[bufIndex]) == -1){
				if(!numTimes) {wasInAlready = increment(map, &map->currentPool[bufIndex], 1);}
				else {wasInAlready = increment(map, &map->currentPool[bufIndex], numTimes);}

				if(wasInAlready){
					map->currentPoolIndex = bufIndex;
				}
				else{
					bufIndex = map->currentPoolIndex;
				}


			}
			else{
				bufIndex = map->currentPoolIndex;
			}
			inWordBool = 0;
			wordStartFileIndex = fileIndex;
			if(!numTimes) {c = fgetc(file);}
			else {return;} //only one word if it is the filename
			++fileIndex;
		}
	}
	if(!numTimes) {fclose(file);}

	/*int i=1;
	for(bufIndex = 0; bufIndex < map->capacity; ++bufIndex){
		if(map->arr[bufIndex].val > 0){
			printf("%i Entries\n%i Table Size\n%s - %i     %i\n", map->numFilled, map->capacity, map->arr[bufIndex].key, getCount(map, map->arr[bufIndex].key), i);
			++i;
		}
	}*/ //<-- will print all entries in hash table after reading this file
}

void printCount(int numToPrint, Map* map, int pipeFd){
	if(numToPrint == -1){ //signals child, print ALL to pipe
		numToPrint = map->numFilled;
	}
	Pair* array = (Pair *)malloc(sizeof(Pair) * numToPrint);
	if(!array){
		fprintf(stderr, "Malloc failed, terminating program.\n");
		exit(1);
	}
	int topIndex;
	for(topIndex = 0; topIndex < numToPrint; ++topIndex){
		array[topIndex].val = 0;
	}
	topIndex = 0;
	int numOccupied = 0;
	int index = 0;
	int currentLow = 1;
	for(index = 0; index < map->capacity; ++index){
		topIndex = 0;
		if(map->arr[index].val >= currentLow){
			if(numOccupied < numToPrint){
				++numOccupied;
			}
			while(topIndex < numOccupied && (array[topIndex].val > map->arr[index].val || (array[topIndex].val == map->arr[index].val && strcmp(array[topIndex].key,map->arr[index].key) < 0))){
				topIndex++;
			}
			if(topIndex == numOccupied && topIndex < numToPrint){
				array[numOccupied] = map->arr[index];
			}
			else if(topIndex < numToPrint){
				int innerIndex = (numOccupied - 1);
				while(topIndex < innerIndex){
					array[innerIndex] = array[innerIndex - 1];
					--innerIndex;
				}
				array[topIndex] = map->arr[index];				
			}
			if(numOccupied == numToPrint){
				currentLow = array[numToPrint-1].val;
			}
		}
	}
	int writeSize;
	for(index = 0; index < numToPrint; ++index){
		writeSize = 25;
		if(array[index].val > 0){
			char* buff = (char* )malloc(sizeof(char) * writeSize);
			if(!buff){
				fprintf(stderr, "Malloc failed, terminating program.\n");
				exit(1);
			}
			int len = snprintf(buff,writeSize,"%s %i",array[index].key,array[index].val);
			while(len+1 >= writeSize){
				writeSize *= 2;
				char* oldBuff = buff;
				buff = (char* )malloc(sizeof(char) * writeSize);
				len = snprintf(buff,writeSize,"%s %i",array[index].key,array[index].val);
				free(oldBuff);
			}
			if(pipeFd != 1) {
				int numBytes = write(pipeFd,buff,len+1);
				if(numBytes < 0){
					perror("Error on write(). Terminating");
					exit(1);
				}
			}
			else {printf("%s\n",buff);}
			free(buff);
		}
	}
	free(array);
}