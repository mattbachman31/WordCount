#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "shmnames.h"


#define INIT_PIPE_BUF 1024

int main(int argc, char** argv){
	char* login = getlogin();
	if(login == NULL){
		perror("Error on getlogin");
		exit(1);
	}
	if(argc < 5){
		printf("Usage: ./word-count DIR_NAME N STOP_WORDS FILES...\n");
		exit(1);
	}
	int dir = chdir(argv[1]);
	if(dir < 0){
		perror("error on chdir");
		printf("Usage: ./word-count DIR_NAME N STOP_WORDS FILES...\n");
		exit(1);
	}
	int checkNumWords = atoi(argv[2]);
	if(checkNumWords <= 0){
		fprintf(stderr,"Error on atoi\n");
		printf("Usage: ./word-count DIR_NAME N STOP_WORDS FILES...\n");
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



	//Client specific stuff...
	sem_t* clientSemWrite;
	sem_t* clientSemRead;
	sem_t* clientSemDone;

	char clientSemWriteString[strlen(CLIENT_SEM_WRITE) + 32]; //further assumption PID not exceeding 32 base 10 digits
	strcpy(clientSemWriteString, CLIENT_SEM_WRITE);
	strcat(clientSemWriteString, pid_string);

	char clientSemReadString[strlen(CLIENT_SEM_READ) + 32];
	strcpy(clientSemReadString, CLIENT_SEM_READ);
	strcat(clientSemReadString, pid_string);

	char clientSemDoneString[strlen(CLIENT_SEM_DONE) + 32];
	strcpy(clientSemDoneString, CLIENT_SEM_DONE);
	strcat(clientSemDoneString, pid_string);

	char clientShmNameString[strlen(CLIENT_SHM_NAME) + 32];
	strcpy(clientShmNameString, CLIENT_SHM_NAME);
	strcat(clientShmNameString, pid_string);



	char serversemstring[strlen(SERVER_SEM_NAME) + strlen(login) + 2]; //+2 for '/' and NUL term
	serversemstring[0] = '/';
	strcpy(&serversemstring[1], login);
	strcpy(&serversemstring[1+strlen(login)],SERVER_SEM_NAME);

	char requestsemstring[strlen(REQUEST_SEM_NAME) + strlen(login) + 2]; //+2 for '/' and NUL term
	requestsemstring[0] = '/';
	strcpy(&requestsemstring[1], login);
	strcpy(&requestsemstring[1+strlen(login)],REQUEST_SEM_NAME);

	char responsesemstring[strlen(RESPONSE_SEM_NAME) + strlen(login) + 2]; //+2 for '/' and NUL term
	responsesemstring[0] = '/';
	strcpy(&responsesemstring[1], login);
	strcpy(&responsesemstring[1+strlen(login)],RESPONSE_SEM_NAME);

	char shmnamestring[strlen(SHM_NAME) + strlen(login) + 2];
	shmnamestring[0] = '/';
	strcpy(&shmnamestring[1],login);
	strcpy(&shmnamestring[1+strlen(login)], SHM_NAME);


	if((clientSemWrite = sem_open(clientSemWriteString, O_RDWR|O_CREAT, ALL_PERMS, 1)) == SEM_FAILED){
		perror("Error initializing client specific semaphore");
		exit(1);
	}
	if((clientSemRead = sem_open(clientSemReadString, O_RDWR|O_CREAT, ALL_PERMS, 0)) == SEM_FAILED){
		perror("Error initializing client specific semaphore");
		exit(1);
	}
	if((clientSemDone = sem_open(clientSemDoneString, O_RDWR|O_CREAT, ALL_PERMS, 0)) == SEM_FAILED){
		perror("Error initializing client specific semaphore");
		exit(1);
	}

	//Make client memory, mmap it in
	int clientshmfd = shm_open(clientShmNameString, O_CREAT|O_RDWR, ALL_PERMS);
	if(clientshmfd < 0){
		perror("Error opening shared memory segment");
		exit(1);
	}
	if((ftruncate(clientshmfd, SHM_SIZE)) < 0){
		perror("Error on ftruncate");
		exit(1);
	}
	char* clientShmBuf = NULL;
	if((clientShmBuf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, clientshmfd, 0)) == MAP_FAILED){
		perror("Error mmaping clients shm");
		exit(1);
	}



	sem_t* serversem;
	sem_t* requestsem;
	sem_t* responsesem;
	if((serversem = sem_open(serversemstring, O_RDWR)) == SEM_FAILED){
		perror("Error initializing semaphore");
		exit(1);
	}
	if((requestsem = sem_open(requestsemstring, O_RDWR)) == SEM_FAILED){
		perror("Error initializing semaphore");
		exit(1);
	}
	if((responsesem = sem_open(responsesemstring, O_RDWR)) == SEM_FAILED){
		perror("Error initializing semaphore");
		exit(1);
	}
	int shmfd = shm_open(shmnamestring, O_RDWR, 0);
	if(shmfd < 0){
		perror("Error opening shared memory segment");
		exit(1);
	}
	char* shmBuf = NULL;
	if((shmBuf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED){
		perror("Error mmaping shm");
		exit(1);
	}

	sem_wait(serversem);
	int shmBufIndex = 0;
	while(shmBufIndex < bufIndex){
		strcpy(&shmBuf[shmBufIndex], &buf[shmBufIndex]);
		shmBufIndex += (strlen(&buf[shmBufIndex]) + 1);
	}
	shmBuf[shmBufIndex] = '\0';


	sem_post(requestsem);
	sem_wait(responsesem);
	sem_post(serversem);

	int prevNullChar = 0;

	//read will go here
	while(1){
		sem_wait(clientSemRead);
		int numReadIndex = 0;
		char c = clientShmBuf[numReadIndex];
		while(!(prevNullChar && c == '\0') && numReadIndex < SHM_SIZE){
			prevNullChar = (c == '\0') ? 1 : 0;
			printf("%c",c);
			if(prevNullChar){
				printf("\n");
			}
			++numReadIndex;
			c = clientShmBuf[numReadIndex];
		}
		int done = sem_trywait(clientSemDone);
		if(done == 0) {break;}
		else{
			if(errno != EAGAIN){
				perror("Error on sem_trywait");
				exit(1);
			}
		}
		sem_post(clientSemWrite);
	}
	if(shm_unlink(clientShmNameString) < 0){ //ensures removal after
		perror("Error on shm_unlink");
		exit(1);
	}
	
	if((close(clientshmfd)) < 0){
		perror("Error closing");
		exit(1);
	}
	if((close(shmfd)) < 0){
		perror("Error closing");
		exit(1);
	}
	if(munmap(shmBuf, SHM_SIZE) < 0){
		perror("Error on munmap");
		exit(1);
	}
	if(munmap(clientShmBuf, SHM_SIZE) < 0){
		perror("Error on munmap");
		exit(1);
	}
	if(sem_close(clientSemRead) < 0){
		perror("Error on sem_close");
		exit(1);
	}
	if(sem_unlink(clientSemReadString) < 0){
		perror("Error on sem_unlink");
		exit(1);
	}
	if(sem_close(clientSemWrite) < 0){
		perror("Error on sem_close");
		exit(1);
	}
	if(sem_unlink(clientSemWriteString) < 0){
		perror("Error on sem_unlink");
		exit(1);
	}
	if(sem_close(clientSemDone) < 0){
		perror("Error on sem_close");
		exit(1);
	}
	if(sem_unlink(clientSemDoneString) < 0){
		perror("Error on sem_unlink");
		exit(1);
	}

	if(sem_close(serversem) < 0){
		perror("Error on sem_close");
		exit(1);
	}
	if(sem_close(requestsem) < 0){
		perror("Error on sem_close");
		exit(1);
	}
	if(sem_close(responsesem) < 0){
		perror("Error on sem_close");
		exit(1);
	}

	free(buf);
	return 0;
}