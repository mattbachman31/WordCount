#ifndef COUNT_H
#define COUNT_H

#include "word-store.h"

void updateWithFile(Map* map, char* fileName, Map* noMap, int numTimes);

void printCount(int numToPrint, Map* map, int pipeFd);

#endif
