#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>


#define MAX_PROGRAMS 100
#define SHM_SIZE 1024
#define MAX_VALUE_SIZE 1064
#define BUFFER_SIZE 10

typedef struct {
    char *name;
    int optional;
} Programs;

// typedef struct {
//     char data[BUFFER_SIZE][MAX_VALUE_SIZE];
//     int in;
//     int out;
//     int done;
// }RingBuffer;
void synchronous_shared_memory(int num_programs, Programs programs[], int buffer_size, int optional) {
    typedef struct {
            char data[buffer_size][MAX_VALUE_SIZE];
            int in;
            int out;
            int done;
    }RingBuffer;

    // key_t shm_key = ftok(".",'R');
    //Create a id for observe and reconstruct
    int shm_id = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    //printf("ID 1 %d",shm_id);
    // Create shared memory segment
    if (shm_id < 0) {
        perror("shmget meme");
        exit(1);
    }
    //Create a id for reconstruct and tapplot
    int shm_id_2 = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    //printf("ID 2 %d",shm_id_2);
    // Create shared memory segment
    if (shm_id_2 < 0) {
        perror("shmget meme");
        exit(1);
    }
    // pid_t pids[MAX_PROGRAMS];
    char optional_argument[256];
    char buff_size[256];
    snprintf(optional_argument,sizeof(optional_argument),"%d",optional);
    snprintf(buff_size,sizeof(buff_size),"%d",buffer_size);
    char buff_option[10] = "sync";
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
    // printf("%d",observe_pid);
    if (observe_pid == 0){
        execl(programs[0].name,"observe",buff_size,optional_argument,"sync",shm_id_obs_rec,NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    pid_t reconstruct_pid = fork();
    if (reconstruct_pid == 0){
        execl(programs[1].name,"reconstruct",buff_size,optional_argument,"sync",shm_id_obs_rec,shm_id_rec_tap,NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    pid_t tapplot_pid = fork();
    if (tapplot_pid == 0){
        execl(programs[2].name,"tapplot",buff_size,optional_argument,"sync",shm_id_obs_rec,shm_id_rec_tap,NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }
    // // Wait for child processes to complete
    waitpid(observe_pid ,NULL, 0);
    waitpid(reconstruct_pid, NULL, 0);
    waitpid(tapplot_pid, NULL, 0);
    // for (int i = 0; i < num_programs ; i++){
    //     // pid_t pid = fork();
    //     pids[i] = fork();
    //     if (pids[i] == 0){ //child process
    //         execl(programs[i].name, programs[i].name,buff_size,optional_argument,"sync",shm_id_obs_rec,shm_id_rec_tap, NULL);
    //         perror("execl");
    //         exit(EXIT_FAILURE);
    //     }else if (pids[i] < 0){
    //         fprintf(stderr, "Error forking");
    //         exit(EXIT_FAILURE);
    //     }
    // }
    // Wait for child processes to complete
    // for (int i = 0 ; i < num_programs ; i++){
    //     waitpid(pids[i], NULL, 0);
    // }
    //Detach shared memory segment
    // if (shmdt(ring_buffer) == -1) {
    //     perror("shmdt");
    //     exit(1);
    // }

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
    typedef struct {
        char data[4][MAX_VALUE_SIZE];
        int in;
        int out;
        sem_t mutex;
        sem_t empty_slots;
        sem_t full_slots;
        int done;
    } Four_Slot_Buffer;

    key_t shm_key = ftok(".",'R');
    
    int shm_id = shmget(shm_key,sizeof(Four_Slot_Buffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id < 0) {
        perror("shmget meme");
        exit(1);
    }
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*)shmat(shm_id,NULL,0);
    if (four_slot_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }

    //initialize four-slot buffer
    four_slot_buffer->in = 0;
    four_slot_buffer->out = 0;
    four_slot_buffer->done = 0;
    //Initialize the sempahores with sem_init
    sem_init(&four_slot_buffer->mutex,1,1);
    sem_init(&four_slot_buffer->empty_slots,1,4);
    sem_init(&four_slot_buffer->full_slots,1,0);
    pid_t pids[MAX_PROGRAMS];

    char optional_argument[256];
    char buff_size[256];
    snprintf(optional_argument,sizeof(optional_argument),"%d",optional);
    snprintf(buff_size,sizeof(buff_size),"%d",buffer_size);
    char buff_option[10] = "async";
    //Fork and execute observe and reconstruct processes
    for (int i = 0; i < num_programs ; i++){
        // pid_t pid = fork();
        pids[i] = fork();
        
        if (pids[i] == 0){ //child process
    
            execl(programs[i].name, programs[i].name,buff_size,optional_argument,"async",NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }else if (pids[i] < 0){
            fprintf(stderr, "Error forking");
            exit(EXIT_FAILURE);
        }
    }
    // Wait for child processes to complete
    for (int i = 0 ; i < num_programs ; i++){
        waitpid(pids[i], NULL, 0);
    }
    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);
    //Detach shared memory segment
    if (shmdt(four_slot_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Delete shared memory segment
    //IPC_RMID Flag will mark the shared memory segment to be destroyed
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
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
    //printf("tapper -p1 %s -p2 %s -p3 %s -o %d -b %d -s %d", program1,program2,program3,optional,is_sync,buffer_size);
    if (is_sync){
        synchronous_shared_memory(num_programs,programs,buffer_size,optional);
    }else{
        asynchronous_shared_memory(num_programs,programs,buffer_size,optional);
    }
    return 0;
}