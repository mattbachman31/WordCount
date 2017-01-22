#include "word-count.h"

void updateWithFile(Map* map, char* fileName, Map* noMap){
	FILE* file = fopen(fileName, "r");
	if(file == NULL){
		fprintf(stderr, "Could not open file: %s\n", fileName);
		printf("Usage: ./word-count DIR_NAME N STOP_WORDS FILES...\n");
		exit(1);
	}
	int bufIndex = map->currentPoolIndex;
	int fileIndex = 0;
	int wordStartFileIndex = 0;
	char c = fgetc(file);
	++fileIndex;
	char inWordBool = 0;
	top: while(c != EOF){
		while(!isalnum(c) && c != '\'' && c != EOF){
			c = fgetc(file);
			++fileIndex;
			++wordStartFileIndex;
		}
		while(isalnum(c) || c == '\''){
			if(map->currentPoolIndex >= map->currentPoolCapacity-1){
				if(bufIndex == 0){
					//current alloc size not big enough for word...
					while(isalnum(c) || c == '\''){
						c = fgetc(file);
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
					fseek(file, wordStartFileIndex, SEEK_SET);
					fileIndex = wordStartFileIndex;
					c = fgetc(file);
					++fileIndex;
					while(isalnum(c) || c == '\''){
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
}

void printCount(int numToPrint, Map* map, IPC_Info ipc_info){
	sem_wait(ipc_info.clientSemWrite);
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
	int ipcIndex = 0;
	char* toPipe = (char *)malloc(toPipeSize);
	if(!toPipe){
		perror("Malloc");
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
					exit(1);
				}
				free(temp);
			}
			int lengthToWrite = (1 + strlen(toPipe));
			int innerIndex = 0;
			while(innerIndex < lengthToWrite){
				if(ipcIndex == ipc_info.sharedSize){
					sem_post(ipc_info.clientSemRead);
					sem_wait(ipc_info.clientSemWrite);
					ipcIndex = 0; //check if word is longer entirely...last case
				}
				ipc_info.sharedBuf[ipcIndex] = toPipe[innerIndex];
				++innerIndex;
				++ipcIndex;
			}
		}
	}
	if(ipcIndex == ipc_info.sharedSize){
		sem_post(ipc_info.clientSemRead);
		sem_wait(ipc_info.clientSemWrite);
		ipcIndex = 0; //check if word is longer entirely...last case
	}
	ipc_info.sharedBuf[ipcIndex] = '\0';
	free(toPipe);
	free(array);
	sem_post(ipc_info.clientSemDone);
	sem_post(ipc_info.clientSemRead);
}