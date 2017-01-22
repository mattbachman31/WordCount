#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "word-count.h"

#define FIFO_NAME "prj3fifo"
#define INIT_PIPE_BUF 4096

int main(int argc, char** argv){
	if(argc != 2){
		printf("Usage: %s DIR_NAME\n",argv[0]);
		exit(1);
	}
	pid_t pid;
	if((pid = fork()) < 0){
		perror("Error on fork()");
		exit(-1);
	}
	if(pid > 0){
		printf("%i\n",pid);
		exit(1); //parent
	}
	else{
		//daemon
		int s = setsid();
		if(s < 0){
			perror("error on setsid");
			exit(1);
		}
		s = chdir(argv[1]);
		if(s < 0){
			perror("error on chdir");
			exit(1);
		}
		if((mkfifo(FIFO_NAME,0777)) < 0 && errno != EEXIST){
			perror("Error on mkfifo");
			exit(1);
		}
		int fdInit = open(FIFO_NAME, O_RDONLY|O_NONBLOCK); //open this nonblocking so write doesnt block, making the next read not block
		if(fdInit < 0){
			perror("Error opening nonblock read end of fifo");
			exit(1);
		}
		int fd1 = open(FIFO_NAME,O_WRONLY);
		if(fd1 < 0){
			perror("Error opening write end of fifo");
			exit(1);
		}
		int fd2 = open(FIFO_NAME,O_RDONLY);
		if(fd2 < 0){
			perror("Error opening read end of fifo");
			exit(1);
		}
		int pipeRead = INIT_PIPE_BUF;
		char* buf = (char *)malloc(pipeRead);
		if(buf == NULL){
			perror("Error on malloc");
			exit(1);
		}
		int numBytes;
		while((numBytes = read(fd2,buf,pipeRead)) > 0){
			while(numBytes == pipeRead){
				pipeRead *= 2;
				char *temp = (char *)realloc(buf, pipeRead);
				if(temp == NULL){
					perror("Error on malloc");
					exit(1);
				}
				buf = temp;
				int numJustRead = read(fdInit, &buf[pipeRead/2], pipeRead/2);
				if(numJustRead < 0 && errno != EAGAIN){
					perror("Error on read");
					exit(1);
				}
				numBytes += numJustRead;
			}
			int bufIndex = 0;
			while(numBytes >= pipeRead){
				pipeRead *= 2;
				buf = realloc(buf,pipeRead);
				if(buf == NULL){
					perror("Error on server realloc");
					exit(1);
				}
			}
			while(1){
				char* pid_string = buf;
				int clientFifoFd = open(pid_string,O_WRONLY);
				int pipePid = atoi(buf);
				if(pipePid <= 0){
					perror("Error on atoi");
					exit(1);
				}
				int len = strlen(buf) + 1;
				bufIndex += len;
				if(bufIndex >= numBytes){
					perror("Error on read from pipe");
					exit(1);
				}
				char* module = &buf[bufIndex];
				len = strlen(&buf[bufIndex]) + 1;
				bufIndex += len;
				if(bufIndex >= numBytes){
					perror("Error on read from pipe");
					exit(1);
				}
				int numWords = atoi(&buf[bufIndex]);
				if(numWords <= 0){
					perror("Error on atoi");
					exit(1);
				}
				len = strlen(&buf[bufIndex])+1;
				bufIndex += len;
				if(bufIndex >= numBytes){
					perror("Error on read from pipe");
					exit(1);
				}
				char* noWords = &buf[bufIndex];
				len = strlen(&buf[bufIndex])+1;
				bufIndex += len;
				int numFiles = 0;
				int bufIndexDup = bufIndex;
				while(bufIndex < numBytes){
					if(strlen(&buf[bufIndex]) > 0){
						bufIndex += (strlen(&buf[bufIndex])+1);
						++numFiles;
					}
					else {
						bufIndex += 1;
						break;
					}
				}
				char* arr[numFiles];
				bufIndex = bufIndexDup;
				int fileIndex = 0;
				while(bufIndex < numBytes){
					if(strlen(&buf[bufIndex]) > 0){
						arr[fileIndex] = &buf[bufIndex];
						bufIndex += (strlen(&buf[bufIndex])+1);
						++fileIndex;
					}
					else{
						bufIndex += 1;
						break;
					}
				}
				int stat;
				if((pid = fork()) < 0){
					perror("Error on fork");
					exit(1);
				}
				if(pid > 0){
					waitpid(pid, &stat,0);
				}
				else{
					if((pid = fork()) < 0){
						perror("error on fork");
						exit(1);
					}
					if(pid > 0){
						close(clientFifoFd);
						exit(0);
					}
					else{
						Map* noMap = create();
						Map* fileMap = create();
						updateWithFile(noMap, noWords, NULL, clientFifoFd, module);

						int filesToRead = 0;
						while(filesToRead < numFiles){
							updateWithFile(fileMap, arr[filesToRead], noMap, clientFifoFd, module);
							++filesToRead;
						}

						printCount(numWords, fileMap, clientFifoFd); 

						destroy(noMap);
						destroy(fileMap);
					}
					close(clientFifoFd);
					exit(0);
				}
				if(close(clientFifoFd) < 0){
					perror("Error on close");
					exit(1);
				}
				if(bufIndex >= numBytes) {break;}
			}
		}
		if(numBytes < 0){
			perror("Error on read()");
			exit(1);
		}
	}
	return 0;
}