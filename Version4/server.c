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
#include "shmnames.h"

#define FILE_LOCK_NAME ".pid"
#define INIT_PIPE_BUF 4096

int main(int argc, char** argv){
	char* login = getlogin();
	if(login == NULL){
		perror("Error on getlogin");
		exit(1);
	}
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
		exit(0); //parent
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
		int fd = open(FILE_LOCK_NAME, O_WRONLY|O_CREAT, 0777);
		if(fd < 0){
			perror("Error opening .pid file");
			exit(1);
		}
		struct flock flock;
		flock.l_type = F_WRLCK;
		flock.l_whence = SEEK_SET;
		flock.l_start = 0;
		flock.l_len = 0;
		if(fcntl(fd, F_SETLK, &flock) < 0){
			if(errno == EAGAIN && fcntl(fd, F_GETLK, &flock) >= 0){
				fprintf(stderr, "File lock held by PID %ld\n", (long)flock.l_pid);
				exit(1);
			}
		}
		long filePid = (long)getpid();
		char pidBuf[256];
		int bytesReturned = snprintf(pidBuf, 256, "%ld\n",filePid);
		bytesReturned = write(fd, pidBuf, bytesReturned+1);
		if(bytesReturned < 0){
			perror("Error on write to .pid file");
			exit(1);
		}

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

		sem_t* serversem;
		sem_t* requestsem;
		sem_t* responsesem;
		int shmfd = shm_open(shmnamestring, O_CREAT|O_RDWR, ALL_PERMS);
		if(shmfd < 0){
			perror("Error opening shared memory segment");
			exit(1);
		}
		if((ftruncate(shmfd, SHM_SIZE)) < 0){
			shm_unlink(SHM_NAME);
			perror("Error on ftruncate");
			exit(1);
		}
		char* shmBuf = NULL;
		if((shmBuf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED){
			perror("Error mmaping shm");
			exit(1);
		}
		if((serversem = sem_open(serversemstring, O_RDWR|O_CREAT, ALL_PERMS, 1)) == SEM_FAILED){
			perror("Error initializing semaphore");
			exit(1);
		}
		if((requestsem = sem_open(requestsemstring, O_RDWR|O_CREAT, ALL_PERMS, 0)) == SEM_FAILED){
			perror("Error initializing semaphore");
			exit(1);
		}
		if((responsesem = sem_open(responsesemstring, O_RDWR|O_CREAT, ALL_PERMS, 0)) == SEM_FAILED){
			perror("Error initializing semaphore");
			exit(1);
		} 

		while(sem_wait(requestsem) == 0){
			int bufIndex = 0;
			char shmBufDup[SHM_SIZE]; //needed for case where next request overwrites your words
			memcpy(shmBufDup, shmBuf, SHM_SIZE);
			//while(1){
				char* pid_string = shmBufDup;
				int pipePid = atoi(shmBufDup);
				if(pipePid <= 0){
					fprintf(stderr, "Error on atoi\n");
					exit(1);
				}
				int len = strlen(shmBufDup) + 1;
				bufIndex += len;

				
				int numWords = atoi(&shmBufDup[bufIndex]);
				if(numWords <= 0){
					fprintf(stderr, "Error on atoi\n");
					exit(1);
				}
				len = strlen(&shmBufDup[bufIndex])+1;
				bufIndex += len;
				char* noWords = &shmBufDup[bufIndex];
				len = strlen(&shmBufDup[bufIndex])+1;
				bufIndex += len;
				int numFiles = 0;
				int bufIndexDup = bufIndex;
				while(1){
					if(strlen(&shmBufDup[bufIndex]) > 0){
						bufIndex += (strlen(&shmBufDup[bufIndex])+1);
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
				while(1){
					if(strlen(&shmBufDup[bufIndex]) > 0){
						arr[fileIndex] = &shmBufDup[bufIndex];
						bufIndex += (strlen(&shmBufDup[bufIndex])+1);
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
						if(munmap(shmBuf, SHM_SIZE) < 0){
							perror("Error on munmap");
							exit(1);
						}
						if(close(shmfd) < 0){
							perror("Error on close");
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
						exit(0);
					}
					else{


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


						sem_t* clientSemRead;
						sem_t* clientSemWrite;
						sem_t* clientSemDone;
						if((clientSemWrite = sem_open(clientSemWriteString, O_RDWR)) == SEM_FAILED){
							perror("Error initializing semaphore");
							exit(1);
						}
						if((clientSemRead = sem_open(clientSemReadString, O_RDWR)) == SEM_FAILED){
							perror("Error initializing semaphore");
							exit(1);
						}
						if((clientSemDone = sem_open(clientSemDoneString, O_RDWR)) == SEM_FAILED){
							perror("Error initializing semaphore");
							exit(1);
						}

						int clientshmfd = shm_open(clientShmNameString, O_RDWR, 0);
						if(clientshmfd < 0){
							perror("Error opening shared memory segment");
							exit(1);
						}
						char* clientShmBuf = NULL;
						if((clientShmBuf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, clientshmfd, 0)) == MAP_FAILED){
							perror("Error mmaping shm");
							exit(1);
						}

						sem_post(responsesem);


						Map* noMap = create();
						Map* fileMap = create();
						updateWithFile(noMap, noWords, NULL);

						int filesToRead = 0;
						while(filesToRead < numFiles){
							updateWithFile(fileMap, arr[filesToRead], noMap);
							++filesToRead;
						}

						IPC_Info ipc_info;
						ipc_info.sharedBuf = clientShmBuf;
						ipc_info.sharedSize = SHM_SIZE;
						ipc_info.clientSemRead = clientSemRead;
						ipc_info.clientSemWrite = clientSemWrite;
						ipc_info.clientSemDone = clientSemDone;

						printCount(numWords, fileMap, ipc_info);

						destroy(noMap);
						destroy(fileMap);

						if(close(clientshmfd) < 0){
							perror("Error on close");
							exit(1);
						}
						if(sem_close(clientSemRead)< 0){
							perror("Error on sem_close");
							exit(1);
						}
						if(sem_close(clientSemWrite) < 0){
							perror("Error on sem_close");
							exit(1);
						}
						if(sem_close(clientSemDone) < 0){
							perror("Error on sem_close");
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
						if(munmap(clientShmBuf, SHM_SIZE) < 0){
							perror("Error on munmap");
							exit(1);
						}
					}
					if(munmap(shmBuf, SHM_SIZE) < 0){
						perror("Error on munmap");
						exit(1);
					}
					if(close(shmfd) < 0){
						perror("Error on close");
						exit(1);
					}
					exit(0);
				}
				//if(bufIndex >= numBytes) {break;}
			//}
		}
	}
	return 0;
}