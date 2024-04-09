#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define BUFFER_SIZE 10
#define MAX_VALUE_SIZE 1064

typedef struct {
    char data[BUFFER_SIZE][MAX_VALUE_SIZE];
    int in;
    int out;
    int done;
} RingBuffer;


int main(){
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
        sleep(2);
        
 
    }
    printf("\n");
    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
    
    return 0;
}
