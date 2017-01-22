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

	int numWords = atoi(argv[1]);
	if(numWords <= 0){
		fprintf(stderr, "N must be a positive integer > 0. Proper Usage: ./word-count N STOP_WORDS FILE1...\n");
		exit(1);
	}

	Map* noMap = create();

	char* fileName = argv[2];
	updateWithFile(noMap, fileName, NULL, 0);


	int numChildren = argc-3;
	int fd[numChildren][2];
	int childFdIndex = -1; //increment (once looping) after fork call so each child has proper val
	int status;
	int filesToRead = 2; //gets incremented first time to three
	while(numChildren > 0){
		++filesToRead;
		--numChildren;
		++childFdIndex;
		if(pipe(fd[childFdIndex]) < 0){
			perror("Error opening pipe");
			exit(1);
		}
		pid_t pid;
		if((pid = fork()) < 0){
			perror("Error on fork()");
			exit(1);
		}
		else if(pid == 0){
			//child
			close(fd[childFdIndex][0]);
			Map* fileMap = create();
			fileName = argv[filesToRead];
			++filesToRead;
			updateWithFile(fileMap, fileName, noMap, 0);
			printCount(-1, fileMap, fd[childFdIndex][1]); //now goes to pipe, -1 is sentinel val
			destroy(fileMap);
			destroy(noMap);
			exit(0);
		}
		else{
			close(fd[childFdIndex][1]);
		}
	}
	Map* parentMap = create();
	numChildren = argc-3;
	childFdIndex = 0;
	int bufferSize;

	while(childFdIndex < numChildren){
		bufferSize = 5000;
		char* buf = (char* )malloc(sizeof(char) * bufferSize);
		if(!buf){
			fprintf(stderr, "Malloc failed, terminating program.\n");
			while(numChildren){ //waits for children to terminate before exiting
				wait(&status);
				--numChildren;
			}
			exit(1);
		}
		int n = 1; //just to enter loop first time
		while(n > 0){
			n = read(fd[childFdIndex][0],buf,bufferSize);
			if(n < 0){
				perror("Error on read(). Terminating");
				exit(1);
			}
			while(n != 0 && n == bufferSize){
				bufferSize *= 2;
				char* oldbuf = buf;
				buf = (char* )malloc(sizeof(char) * bufferSize);
				if(!buf){
					fprintf(stderr, "Malloc failed, terminating program.\n");
					while(numChildren){ //waits for children to terminate before exiting
						wait(&status);
						--numChildren;
					}
					exit(1);
				}
				for(int loop = 0; loop < (bufferSize/2); ++loop){
					buf[loop] = oldbuf[loop];
				}
				free(oldbuf);
				n += read(fd[childFdIndex][0],(buf+bufferSize/2),bufferSize/2);
			}
			int numRead = 0;
			while(numRead < n){
				int numToAddToBuf = strlen(buf+numRead) + 1; //need since we're adding a newline
				char* c = buf+numRead;
				while(*c != ' ') {
					++c;
				}
				*c = '\0';
				++c;
				int numTimes = atoi(c);
				updateWithFile(parentMap, buf+numRead, noMap, numTimes);
				numRead += numToAddToBuf;
			}
		}
		free(buf);
		++childFdIndex;
	}
	printCount(numWords, parentMap, 1);
	destroy(noMap);
	destroy(parentMap);

	return 0;
}