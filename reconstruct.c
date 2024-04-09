#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>

#define BUFFER_SIZE 10
#define MAX_VALUE_SIZE 1064

// typedef struct {
//     char data[BUFFER_SIZE][MAX_VALUE_SIZE];
//     int in;
//     int out;
//     int done;
// } RingBuffer;

void sync_reconstruct(int buffer_size){
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;
    key_t shm_key = ftok(".",'R');
    int shm_id = shmget(shm_key,sizeof(RingBuffer), 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }
    /* Attach shared memory segment to shared_data */
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id,NULL,0);

    if (ring_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }

    while(1){
        // Read observed changes from the ring buffer
        while(ring_buffer->in == ring_buffer->out && !ring_buffer->done);//wait if buffer is empty
        
        if (ring_buffer->done && ring_buffer->in == ring_buffer->out){//Finished reading all the data in the slots
            break;
        }
        
        printf("Consumer produce %s\n", ring_buffer->data[ring_buffer->out]);

        ring_buffer->out = (ring_buffer->out + 1) % BUFFER_SIZE;//Move on the next name value pair to read
        // sleep(2);
        
 
    }
    printf("\n");
    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void async_reconstruct(){
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
    int shm_id = shmget(shm_key,sizeof(Four_Slot_Buffer),0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }
    /* Attach shared memory segment to shared_data */
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*) shmat(shm_id,NULL,0);

    if (four_slot_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
    while(1){
        sem_wait(&four_slot_buffer->full_slots);
        sem_wait(&four_slot_buffer->mutex);
        if (four_slot_buffer->done && four_slot_buffer->out == four_slot_buffer->in){
            sem_post(&four_slot_buffer->mutex);
            sem_post(&four_slot_buffer->empty_slots);
            break;

        }
        printf("Consumer produce %s\n", four_slot_buffer->data[four_slot_buffer->out]);
        four_slot_buffer->out = (four_slot_buffer->out + 1) % 4;
        //sleep(2);
        sem_post(&four_slot_buffer->mutex);
        sem_post(&four_slot_buffer->empty_slots);


        
    }

    printf("Done \n");
    //Detach shared memory segment
    if (shmdt(four_slot_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
    
}
int main(int argc, char *argv[]){
    int buffer_size = atoi(argv[1]);
    char *buffer_option = argv[3];
    if (strcmp(buffer_option,"sync") == 0){
        sync_reconstruct(buffer_size);
    }else{
        async_reconstruct();
    }
    return 0;
}
