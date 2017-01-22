#include "word-count.h"

void updateWithFile(Map* map, char* fileName, Map* noMap, int fd, char* funcName){
	typedef int (IsWordChar)(int c);
	FILE* file = fopen(fileName, "r");
	char errorBuf[2];
	errorBuf[0] = '\0';
	errorBuf[1] = '\0';
	if(file == NULL){
		fprintf(stderr, "Could not open file: %s\n", fileName);
		printf("Usage: ./word-count DIR_NAME WORD_CHAR_MODULE N STOP_WORDS FILES...\n");
		write(fd, errorBuf, 2);
		exit(1);
	}
	void *dlHandle = dlopen(funcName, RTLD_NOW);
	if(dlHandle == NULL){
		perror("Error on dlopen");
		write(fd, errorBuf, 2);
		exit(1);
	}
	IsWordChar* isWordChar = dlsym(dlHandle, IS_WORD_CHAR);
	if(isWordChar == NULL){
		perror("Error on dlsym");
		write(fd, errorBuf, 2);
		exit(1);
	}
	int bufIndex = map->currentPoolIndex;
	int fileIndex = 0;
	int wordStartFileIndex = 0;
	char c = fgetc(file);
	++fileIndex;
	char inWordBool = 0;
	top: while(c != EOF){
		while(!isWordChar(c) && c != EOF){
			c = fgetc(file);
			++fileIndex;
			++wordStartFileIndex;
		}
		while(isWordChar(c)){
			if(map->currentPoolIndex >= map->currentPoolCapacity-1){
				if(bufIndex == 0){
					//current alloc size not big enough for word...
					while(isWordChar(c)){
						c = fgetc(file);
						++fileIndex;
					}
					int wordLength = (fileIndex - wordStartFileIndex); //null-terminating included in last ++fileIndex
					char* newBigPool = (char *)malloc(sizeof(char) * wordLength);
					if(!newBigPool){
						fprintf(stderr, "Malloc failed, terminating program.\n");
						write(fd, errorBuf, 2);
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
							write(fd, errorBuf, 2);
							exit(1);
						}
					}
					fseek(file, wordStartFileIndex, SEEK_SET);
					fileIndex = wordStartFileIndex;
					c = fgetc(file);
					++fileIndex;
					while(isWordChar(c)){
						map->currentPool[map->currentPoolIndex] = tolower(c);
						++map->currentPoolIndex;
						c = fgetc(file);
						++fileIndex;
					}
					goto bottom;
				}
				else{
					//allocate new pool
					char* newPool = (char *)malloc(sizeof(char) * POOL_ALLOC_SIZE);
					if(!newPool){
						fprintf(stderr, "Malloc failed, terminating program.\n");
						write(fd, errorBuf, 2);
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
							write(fd, errorBuf, 2);
							exit(1);
						}
					}
					fseek(file, wordStartFileIndex, SEEK_SET);
					fileIndex = wordStartFileIndex;
					c = fgetc(file);
					++fileIndex;
					goto top;
				}
			}
			inWordBool = 1;
			map->currentPool[map->currentPoolIndex] = tolower(c);
			++map->currentPoolIndex;

			c = fgetc(file);
			++fileIndex;
		}
		bottom: if(inWordBool){
			map->currentPool[map->currentPoolIndex] = '\0';
			++map->currentPoolIndex;
			
			int wasInAlready;
			if(noMap == NULL || getCount(noMap, &map->currentPool[bufIndex]) == -1){
				wasInAlready = increment(map, &map->currentPool[bufIndex]);

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
			c = fgetc(file);
			++fileIndex;
		}
	}
	fclose(file);

	/*int i=1;
	for(bufIndex = 0; bufIndex < map->capacity; ++bufIndex){
		if(map->arr[bufIndex].val > 0){
			printf("%i Entries\n%i Table Size\n%s - %i     %i\n", map->numFilled, map->capacity, map->arr[bufIndex].key, getCount(map, map->arr[bufIndex].key), i);
			++i;
		}
	}*/ //<-- will print all entries in hash table after reading this file
}

void printCount(int numToPrint, Map* map, int fd){
	char errorBuf[2];
	errorBuf[0] = '\0';
	errorBuf[1] = '\0';
	Pair* array = (Pair *)malloc(sizeof(Pair) * numToPrint);
	if(!array){
		perror("Malloc");
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
			while(topIndex < numOccupied && (array[topIndex].val > map->arr[index].val || (array[topIndex].val == map->arr[index].val && strcmp(array[topIndex].key,map->arr[index].key) > 0))){
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
	int toPipeSize = 256;
	int writeBufferSize = 256, writeBufferIndex = 0;
	char* toPipe = (char *)malloc(toPipeSize);
	if(!toPipe){
		perror("Malloc");
		write(fd, errorBuf, 2);
		exit(1);
	}
	char* writeBuffer = (char *)malloc(writeBufferSize);
	if(!writeBuffer){
		perror("Malloc");
		write(fd, errorBuf, 2);
		exit(1);
	}
	for(index = 0; index < numToPrint; ++index){
		if(array[index].val > 0){
			while(snprintf(toPipe, toPipeSize, "%s %i", array[index].key, array[index].val) >= toPipeSize){
				char* temp = toPipe;
				toPipeSize *= 2;
				toPipe = (char *)malloc(toPipeSize);
				if(!toPipe){
					perror("Malloc");
					write(fd, errorBuf, 2);
					exit(1);
				}
				free(temp);
			}
			int lengthToWrite = (1 + strlen(toPipe));
			while(lengthToWrite + writeBufferIndex >= writeBufferSize){
				writeBufferSize *= 2;
				char* temp = (char *)realloc(writeBuffer, writeBufferSize);
				if(!temp){
					perror("Malloc");
					write(fd, errorBuf, 2);
					exit(1);
				}
				writeBuffer = temp;
			}
			int innerIndex = 0;
			while(innerIndex < lengthToWrite){
				writeBuffer[writeBufferIndex] = toPipe[innerIndex];
				++innerIndex;
				++writeBufferIndex;
			}
		}
	}
	if(writeBufferIndex == 0){
		writeBuffer[0] = '\0';
		if((write(fd, writeBuffer, 1)) < 0){
			perror("Error on write");
			write(fd, errorBuf, 2);
			exit(1);
		}
	}
	else if((write(fd,writeBuffer, writeBufferIndex)) < 0){
		perror("Error on write");
		write(fd, errorBuf, 2);
		exit(1);
	}
	free(writeBuffer);
	free(toPipe);
	free(array);
}