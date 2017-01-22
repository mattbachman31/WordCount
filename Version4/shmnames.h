#include <semaphore.h>
#include <sys/mman.h>

#define IPC_PREFIX "/mbachma1-"

#define SHM_NAME "-shm"
#define SHM_SIZE 4096
#define SERVER_SEM_NAME "-server-sem"
#define REQUEST_SEM_NAME "-request-sem"
#define RESPONSE_SEM_NAME "-response-sem"
#define ALL_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define CLIENT_SEM_WRITE IPC_PREFIX "client-write-"
#define CLIENT_SEM_READ IPC_PREFIX "client-read-"
#define CLIENT_SEM_DONE IPC_PREFIX "done-"
#define CLIENT_SHM_NAME IPC_PREFIX "client-shm-"