#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>

#define MAX_VALUE_SIZE 1024
#define MAX_FIELD_LENGTH 256

int main(int argc, char *argv[]) {
    sleep(10);
    printf("Before variables ");
    int buffer_size = atoi(argv[1]);
    int argn = atoi(argv[2]);
    char *buffer_option = argv[3];
    int shm_id = atoi(argv[4]);
    int shm_id_2 = atoi(argv[5]);
    printf("THIS IS TAPPLOT %d\n", shm_id_2);
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id_2,NULL,0);

    if (ring_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
    printf("Before the while loop waiting \n");
    while (ring_buffer->in == ring_buffer->out && !ring_buffer->done) {
        printf("Waiting one second");
        if (ring_buffer->in == ring_buffer->out){//Finished reading all the data in the slots
            // Check if the producer is done
            if (ring_buffer->done) {
                break;
            }
            // Buffer is empty, wait for the producer to produce more data
            sleep(1);
            continue;
        }
        char *result = strdup(ring_buffer->data[ring_buffer->out]);
        printf("Consumer produce %s\n",result);
        ring_buffer->out = (ring_buffer->out + 1) % buffer_size;//Move on the next name value pair to read
        sleep(1);
    }
    printf("Ready to read ring buffer");
    //char *result = strdup(ring_buffer->data[ring_buffer->out]);
    //printf("Consumer produce %s\n",result);
    while(1){
        
        
      
    }       
        
    
    FILE *gnuplot_pipe = popen("gnuplot -persist", "w");
    // if (gnuplot_pipe == NULL) {
    //     perror("Error opening gnuplot pipe");
    //     exit(1);
    // }

    // fprintf(gnuplot_pipe, "set title 'Plot'\n");
    // fprintf(gnuplot_pipe, "set xlabel 'Sample Number'\n");
    // fprintf(gnuplot_pipe, "set ylabel 'Value'\n");
    // fprintf(gnuplot_pipe, "plot 'data.txt' with lines\n");

    // pclose(gnuplot_pipe);
    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}