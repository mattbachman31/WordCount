#include "word-count.h"

int main(int argc, char** argv){

	if(argc < 4){
		fprintf(stderr, "Not enough arguments (at least 4). Proper usage: ./word-count N STOP_WORDS FILE1...\n");
		exit(1);
	}

	char* shouldBeNum = argv[1];
	int a = 0;
	char c = shouldBeNum[a];
	while(c != '\0'){
		if(!isdigit(c)){
			fprintf(stderr, "N must be a positive integer > 0. Proper usage: ./word-count N STOP_WORDS FILE1...\n");
			exit(1);
		}
		++a;
		c = shouldBeNum[a];
	}

	int numTimes = atoi(argv[1]);
	if(numTimes <= 0){
		fprintf(stderr, "N must be a positive integer > 0. Proper Usage: ./word-count N STOP_WORDS FILE1...\n");
		exit(1);
	}

	Map* noMap = create();
	Map* fileMap = create();

	char* fileName = argv[2];
	updateWithFile(noMap, fileName, NULL);

	int filesToRead = 3;
	while(filesToRead < argc){
		fileName = argv[filesToRead];
		++filesToRead;
		updateWithFile(fileMap, fileName, noMap);
	}

	printCount(numTimes, fileMap);

	destroy(noMap);
	destroy(fileMap);

	return 0;
}