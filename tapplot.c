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
#define MAX_SAMPLES 1064
int main(int argc, char *argv[]) {
    int buffer_size = atoi(argv[1]);
    int argn = atoi(argv[2]);
    char *buffer_option = argv[3];
    // int shm_id = atoi(argv[4]);
    int shm_id_2 = atoi(argv[5]);
    
    printf("Tapplot Process: Tapplot Consumer ID 2 %d\n",shm_id_2);
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
    char *constructed_samples[MAX_SAMPLES];
    int num_inputs = 0;
    
    printf("Consumer produces %s\n",ring_buffer->data[ring_buffer->out]);
    // while(1){
    //     // Read observed changes from the ring buffer
    //     while(ring_buffer->in == ring_buffer->out){
    //         sleep(2);
    //     };//wait if buffer is empty
        

    //     char *result = strdup(ring_buffer->data[ring_buffer->out]);
    //     printf("Consumer produces %s\n",ring_buffer->data[ring_buffer->out]);
    //     if (strcmp(result, "END_OF_PRODUCTION") == 0) {
    //         free(result);
    //         break;
    //     }
    //     constructed_samples[num_inputs] = result;
    //     ring_buffer->out = (ring_buffer->out + 1) % buffer_size;//Move on the next name value pair to read
    //     num_inputs++;

    //     //sleep(2); 
    // }
    // for (int i = 0;i < num_inputs ;i++){
    //     printf("%s\n",constructed_samples[i]);
    // }

        
    
    // FILE *gnuplot_pipe = popen("gnuplot -persist", "w");
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
    // for (int i = 0; i < num_inputs; i++){
    //     printf("Consumer produce %s\n",constructed_samples[i]);
    // }
    return 0;
}