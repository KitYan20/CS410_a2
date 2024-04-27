#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_PROGRAMS 100
#define MAX_VALUE_SIZE 1064
#define BUFFER_SIZE 10

typedef struct {
    char *name;
} Programs;
//Create a Four Slot Asynchronous Buffer data structure
typedef struct {
    char data[4][MAX_VALUE_SIZE];//Initiate our 4 slot array bufferto fill the samples
    int in;//Initiate a pointer for writing a sample in the buffer specified by 'in'
    int out;//Initiate a pointer for reading a sample in the buffer specified by 'out'
    sem_t mutex;//Create a mutex semaphore for locking
    sem_t empty_slots;//Create a empty slots semaphore to be use for binary semaphore
    sem_t full_slots;//Create a full slots semaphore to be use for binary semaphore
    int done;//Initiate a done binary condition
} Four_Slot_Buffer;

void synchronous_shared_memory(int num_programs, Programs programs[], int buffer_size, int optional) {
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];//Create our ring buffer specified by user size to fill the samples
        int in;//Initiate a pointer for writing a sample in the buffer specified by 'in'
        int out;//Initiate a pointer for reading a sample in the buffer specified by 'out'
        int done;//Initiate a done binary condition
    }RingBuffer;

    //Create a shared memory id for observe and reconstruct
    int shm_id = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id < 0) {
        perror("shmget meme");
        exit(1);
    }

    //Create a shared memory id for reconstruct and tapplot
    int shm_id_2 = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id_2 < 0) {
        perror("shmget meme");
        exit(1);
    }
    //These variables will be used as arguments for our child processes
    char optional_argument[256];
    char buff_size[256];
    snprintf(optional_argument,sizeof(optional_argument),"%d",optional);
    snprintf(buff_size,sizeof(buff_size),"%d",buffer_size);
    char shm_id_obs_rec[20];
    char shm_id_rec_tap[20];
    snprintf(shm_id_obs_rec, sizeof(shm_id_obs_rec), "%d", shm_id);
    snprintf(shm_id_rec_tap, sizeof(shm_id_rec_tap), "%d", shm_id_2);

    //Fork and execute observe and reconstruct processes
    // printf("%s",programs[0].name);//observe process
    // printf("%s",programs[1].name);//reconstruct process
    // printf("%s",programs[2].name);//tapplot process

    //fork and execute observe process
    pid_t observe_pid = fork();
    if (observe_pid == 0) {
        //use execl to execute the executable observe process with a list of specified arguments
        execl(programs[0].name, "observe", buff_size, optional_argument, "sync", shm_id_obs_rec, NULL);
        perror("execl");
        exit(1);
    } else if (observe_pid < 0) {
        perror("fork");
        exit(1);
    } 
    
    pid_t reconstruct_pid = fork();
    if (reconstruct_pid == 0) {
        //use execl to execute the executable reconstruct process with a list of specified arguments
        execl(programs[1].name, "reconstruct", buff_size, optional_argument, "sync", shm_id_obs_rec, shm_id_rec_tap, NULL);
        perror("execl");
        exit(1);
    } else if (reconstruct_pid < 0) {
        perror("fork");
        exit(1);
    } 
    waitpid(observe_pid, NULL, 0); // Wait for observe to finish
    waitpid(reconstruct_pid, NULL, 0); // Wait for reconstruct to finish
    
    pid_t tapplot_pid = fork();
    if (tapplot_pid == 0) {
        //use execl to execute the executable tapplot process with a list of specified arguments
        execl(programs[2].name, "tapplot", buff_size, optional_argument, "sync", shm_id_obs_rec, shm_id_rec_tap, NULL);
        perror("execl");
        exit(1);
    } else if (tapplot_pid < 0) {
        perror("fork");
        exit(1);
    } 
    waitpid(tapplot_pid, NULL, 0); // Wait for tapplot to finish

    // Delete shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
    if (shmctl(shm_id_2, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
}

void asynchronous_shared_memory(int num_programs, Programs programs[], int buffer_size, int optional){  
    //Same process as synchronous_shared_memory  
    int shm_id = shmget(IPC_PRIVATE,sizeof(Four_Slot_Buffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }
    //Create a id for reconstruct and tapplot
    int shm_id_2 = shmget(IPC_PRIVATE,sizeof(Four_Slot_Buffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id_2 < 0) {
        perror("shmget");
        exit(1);
    }
    char optional_argument[256];
    char buff_size[256];
    snprintf(optional_argument,sizeof(optional_argument),"%d",optional);
    snprintf(buff_size,sizeof(buff_size),"%d",buffer_size);
    char shm_id_obs_rec[20];
    char shm_id_rec_tap[20];
    snprintf(shm_id_obs_rec, sizeof(shm_id_obs_rec), "%d", shm_id);
    snprintf(shm_id_rec_tap, sizeof(shm_id_rec_tap), "%d", shm_id_2);

    //Fork and execute observe and reconstruct processes
    // printf("%s",programs[0].name);//observe process
    // printf("%s",programs[1].name);//reconstruct process
    // printf("%s",programs[2].name);//tapplot process

    //fork and execute observe process
    pid_t observe_pid = fork();
    if (observe_pid == 0) {
        execl(programs[0].name, "observe", buff_size, optional_argument, "async", shm_id_obs_rec, NULL);
        perror("execl");
        exit(1);
    } else if (observe_pid < 0) {
        perror("fork");
        exit(1);
    } 
    pid_t reconstruct_pid = fork();
    if (reconstruct_pid == 0) {
        execl(programs[1].name, "reconstruct", buff_size, optional_argument, "async", shm_id_obs_rec, shm_id_rec_tap, NULL);
        perror("execl");
        exit(1);
    } else if (reconstruct_pid < 0) {
        perror("fork");
        exit(1);
    } 
    waitpid(observe_pid, NULL, 0); // Wait for observe to finish
    waitpid(reconstruct_pid, NULL, 0); // Wait for reconstruct to finish
    pid_t tapplot_pid = fork();
    if (tapplot_pid == 0) {
        execl(programs[2].name, "tapplot", buff_size, optional_argument, "async", shm_id_obs_rec, shm_id_rec_tap, NULL);
        perror("execl");
        exit(1);
    } else if (tapplot_pid < 0) {
        perror("fork");
        exit(1);
    } 
    waitpid(tapplot_pid, NULL, 0); // Wait for tapplot to finish
    // Delete shared memory segment once every process has been completed 
    //IPC_RMID Flag will mark the shared memory segment to be destroyed
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
    //IPC_RMID Flag will mark the shared memory segment to be destroyed
    if (shmctl(shm_id_2, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
}

int main(int argc, char *argv[]){
    int is_sync = 0;
    int buffer_size = 1;
    int optional = 1;
    int opt;
    Programs programs[MAX_PROGRAMS];
    int num_programs = 0;
    while((opt = getopt(argc,argv, "p:b:s:o:")) != -1){
        switch (opt){
            case 'p':
                programs[num_programs].name= optarg + 2;
                num_programs++;
                break;
            case 'b':
                if (strcmp(optarg,"sync") == 0){
                    is_sync = 1;
                }
                break;
            case 's':
                buffer_size = atoi(optarg);
                break;
            case 'o':
                optional = atoi(optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }

    }
    if (is_sync){
        synchronous_shared_memory(num_programs,programs,buffer_size,optional);
    }else{
        asynchronous_shared_memory(num_programs,programs,buffer_size,optional);
    }
    return 0;
}