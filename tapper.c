#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>

#define MAX_PROGRAMS 4
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

    key_t shm_key = ftok(".",'R');
    
    int shm_id = shmget(shm_key,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    // Create shared memory segment
    if (shm_id < 0) {
        perror("shmget meme");
        exit(1);
    }
    /* Attach shared memory segment to shared_data */    
    RingBuffer *ring_buffer;
    ring_buffer = (RingBuffer*) shmat(shm_id,NULL,0);
    // printf("shm id %d\n",shm_id);
    // printf("ring buffer address 0x%x\n",ring_buffer);

    if (ring_buffer == (void*)-1){//Memory address of the shared memory segment
        perror("Meme shmat");
        exit(1);
    }
    //Initialize the ring buffer
    ring_buffer->in = 0;
    ring_buffer->out = 0;
    ring_buffer->done = 0;

    pid_t pids[MAX_PROGRAMS];

    char optional_argument[256];
    char buff_size[256];
    snprintf(optional_argument,sizeof(optional_argument),"%d",optional);
    snprintf(buff_size,sizeof(buff_size),"%d",buffer_size);
    //Fork and execute observe and reconstruct processes
    for (int i = 0; i < num_programs ; i++){
        // pid_t pid = fork();
        pids[i] = fork();
        
        if (pids[i] == 0){ //child process
    
            execl(programs[i].name, programs[i].name,buff_size,optional_argument, NULL);
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
    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Delete shared memory segment
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
                if (num_programs < MAX_PROGRAMS){
                    programs[num_programs].name= optarg + 2;
                     
                    num_programs++;
                }
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
        printf("Nothing");
    }
    return 0;
}