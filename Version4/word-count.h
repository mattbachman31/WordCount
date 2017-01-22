#ifndef COUNT_H
#define COUNT_H

#include "word-store.h"
#include <semaphore.h>

typedef struct{
	char* sharedBuf;
	int sharedSize;
	sem_t* clientSemRead;
	sem_t* clientSemWrite;
	sem_t* clientSemDone;
} IPC_Info;

void updateWithFile(Map* map, char* fileName, Map* noMap);

void printCount(int numToPrint, Map* map, IPC_Info ipc_info);

#endif
