#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
//#include "word-count.h"

#define FIFO_NAME "prj3fifo"
#define INIT_PIPE_BUF 1024

int main(int argc, char** argv){
	if(argc < 6){
		printf("Usage: ./word-count DIR_NAME WORD_CHAR_MODULE N STOP_WORDS FILES...\n");
		exit(1);
	}
	int dir = chdir(argv[1]);
	if(dir < 0){
		perror("error on chdir");
		printf("Usage: ./word-count DIR_NAME WORD_CHAR_MODULE N STOP_WORDS FILES...\n");
		exit(1);
	}
	int checkNumWords = atoi(argv[3]);
	if(checkNumWords <= 0){
		perror("Error on atoi");
		printf("Usage: ./word-count DIR_NAME WORD_CHAR_MODULE N STOP_WORDS FILES...\n");
		exit(1);
	}
	int fd =  open(FIFO_NAME,O_WRONLY);
	if(fd < 0){
		perror("Error on open");
		exit(1);
	}
	int bufSize = INIT_PIPE_BUF;
	char* buf = (char *)malloc(bufSize);
	if(buf == NULL){
		perror("Error on malloc");
		exit(1);
	}
	int numArgs = argc-2; //not sending executable name or dir name
	int numBytes = 0;
	int i;
	for(i=0; i<numArgs; ++i){
		numBytes += (strlen(argv[i+2])+1);
	}

	numBytes += 1; //for ending double '\0'
	char pid_string[32];
	sprintf(pid_string, "%i", getpid());
	numBytes += (strlen(pid_string) + 1);

	while(numBytes >= bufSize){
		bufSize *= 2;
		buf = realloc(buf, bufSize);
		if(buf == NULL){
			perror("Error on realloc");
			exit(1);
		}
	}
	i=0;
	int bufIndex = 0;
	while(bufIndex < strlen(pid_string)){
		buf[bufIndex] = pid_string[bufIndex];
		++bufIndex;
	}
	buf[bufIndex] = '\0';
	++bufIndex;
	while(i<numArgs){
		int len = strlen(argv[i+2]);
		int j=0;
		while(j < len){
			buf[bufIndex] = argv[i+2][j];
			++j;
			++bufIndex;
		}
		buf[bufIndex] = '\0';
		++bufIndex;
		++i;
	}
	buf[bufIndex] = '\0';

	//set up client specific pipe
	if((mkfifo(pid_string,0777)) < 0 && errno != EEXIST){
		perror("Error on mkfifo");
		exit(1);
	}
	int fdInit = open(pid_string, O_RDONLY|O_NONBLOCK); //open this nonblocking so write doesnt block, making the next read not block
	if(fdInit < 0){
		perror("Error opening nonblock read end of fifo");
		unlink(pid_string);
		exit(1);
	}
	int fd1 = open(pid_string,O_WRONLY);
	if(fd1 < 0){
		perror("Error opening write end of fifo");
		unlink(pid_string);
		exit(1);
	}
	int fd2 = open(pid_string,O_RDONLY);
	if(fd2 < 0){
		perror("Error opening read end of fifo");
		unlink(pid_string);
		exit(1);
	}
	int closeRet = close(fdInit);
	if(closeRet < 0){
		perror("Error closing");
		unlink(pid_string);
		exit(1);
	}

	int w = write(fd, buf, numBytes);
	if(w < 0){
		perror("Error on write");
		unlink(pid_string);
		exit(1);
	}
	int initSize = 256;
	char* bufToPrint = (char *)malloc(initSize);
	if(bufToPrint == NULL){
		perror("Error on malloc");
		unlink(pid_string);
		exit(1);
	}
	int printRead = read(fd2, bufToPrint, initSize);
	while(printRead == initSize){
		initSize *= 2;
		char* temp = (char *)realloc(bufToPrint, initSize);
		if(temp == NULL){
			perror("Error on malloc");
			unlink(pid_string);
			exit(1);
		}
		bufToPrint = temp;
		int numJustRead = read(fd2, &bufToPrint[initSize/2], initSize/2);
		if(numJustRead < 0){
			perror("Error on read");
			unlink(pid_string);
			exit(1);
		}
		printRead += numJustRead;
	}
	if(printRead < 0){
		perror("Error on read");
		unlink(pid_string);
		exit(1);
	}
	else if(printRead == 1){

	}
	else if(printRead == 2 && bufToPrint[0] == '\0' && bufToPrint[1] == '\0'){
		printf("Error reported by worker process\n");
	}
	else{
		int printIndex = 0;
		while(printIndex < printRead){
			printf("%s\n",&bufToPrint[printIndex]);
			printIndex += (strlen(&bufToPrint[printIndex]) + 1);
		}
	}
	if((close(fd1)) < 0){
		perror("Error closing");
		unlink(pid_string);
		exit(1);
	}
	if((close(fd2)) < 0){
		perror("Error closing");
		unlink(pid_string);
		exit(1);
	}
	if((unlink(pid_string)) < 0){
		perror("Error unlinking fifo");
		exit(1);
	}
	return 0;
}