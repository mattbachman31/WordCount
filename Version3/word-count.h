#ifndef COUNT_H
#define COUNT_H

#include "word-store.h"
#define IS_WORD_CHAR "isWordChar"

void updateWithFile(Map* map, char* fileName, Map* noMap, int fd, char* funcName);

void printCount(int numToPrint, Map* map, int fd);

#endif
