#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#define MAX_VALUE_SIZE 1064
#define MAX_SAMPLES 2128

typedef struct {
    char data[4][MAX_VALUE_SIZE];
    int in;
    int out;
    sem_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
    int done;
} Four_Slot_Buffer;

void sync_plot(int buffer_size, int argn, int shm_id_2){
    // printf("Tapplot Process\n");
    // printf("Reconstruct to Tapplot Producer ID 2 %d\n", shm_id_2);
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id_2, NULL, 0);
    if (ring_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    
    printf("\n");
    // Consumer
    
    int num_samples = 0;
    char samples[MAX_SAMPLES][MAX_VALUE_SIZE];
    while (1) {
        while (ring_buffer->in == ring_buffer->out) {// Wait Buffer is empty
            if (ring_buffer->done) {//Indicate all samples been processed
                printf("Consumer 2 finished consuming all samples\n");
                break;
            }
            sleep(2);
        } 
        if (ring_buffer->done && ring_buffer->in == ring_buffer->out) {
            break;
        }

        printf("Consumer 2 consumes %s\n", ring_buffer->data[ring_buffer->out]);
        strcpy(samples[num_samples], ring_buffer->data[ring_buffer->out]);
        num_samples++;
        
        ring_buffer->out = (ring_buffer->out + 1) % buffer_size;  // Move on to the next slot in the buffer using modulo
    }
    // for (int i = 0; i < num_samples ; i++){
    //     printf("%s\n",samples[i]);
    // }
    
    
    // Open a pipe to Gnuplot
    FILE *gnuplot_pipe = popen("gnuplot -persistent", "w");
    if (gnuplot_pipe == NULL) {
        perror("popen");
        exit(1);
    }
     // Set up Gnuplot
    fprintf(gnuplot_pipe, "set title 'Temperature Plot'\n");
    fprintf(gnuplot_pipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplot_pipe, "set ylabel 'Temperature (F)'\n");
    fprintf(gnuplot_pipe, "set grid\n");

    // Write the data to a temporary file
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("tmpfile");
        exit(1);
    }

    for (int i = 0; i < num_samples; i++) {
        char *sample = samples[i];
        char *token = strtok(sample, ",");
        int field_count = 1;
        int value = 0;

        while (token != NULL) {
            if (field_count == argn) {
                char *value_str = strchr(token, '=');
                if (value_str != NULL) {
                    value_str++;  // Skip '='
                    value = atoi(value_str);
                    break;
                }
            }
            token = strtok(NULL, ",");
            field_count++;
        }
        //printf("%d\n", (int)value);
        fprintf(temp_file, "%d %d\n", i + 1, value);
    }

    rewind(temp_file);

    // Plot the data using Gnuplot
    fprintf(gnuplot_pipe, "plot '-' with linespoints title 'Temperature'\n");
    char line[100];
    while (fgets(line, sizeof(line), temp_file) != NULL) {
        fprintf(gnuplot_pipe, "%s", line);
    }
    fprintf(gnuplot_pipe, "e\n");
    fflush(gnuplot_pipe);

    // Close the temporary file
    fclose(temp_file);

    // Close the Gnuplot pipe
    pclose(gnuplot_pipe);
    // Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
    
}
void async_plot(int argn, int shm_id_2){
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*)shmat(shm_id_2, NULL, 0);
    int num_samples = 0;
    char *reconstructed_samples[MAX_SAMPLES];
    if (four_slot_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    printf("Hello\n");
    struct timespec timeout;
    //Consumer
    //Code needs to be FIXED
    // while(1){
    //     clock_gettime(CLOCK_REALTIME, &timeout);
    //     timeout.tv_sec += 1; // Set timeout to 1 second from now
    //     //It will attempt to acquire the 'full_slots' semaphore within the specified timeout meaning there is data to consume
    //     if (sem_timedwait(&four_slot_buffer->full_slots, &timeout) == 0) {
    //         //Attempting to acquire a mutex semaphore 
    //         //If acquired, it'll gain exclusive access to the shared buffer.
    //         sem_wait(&four_slot_buffer->mutex); 
    //         if (four_slot_buffer->done && four_slot_buffer->in == four_slot_buffer->out) {//break out of the consumer once it has finished processed all the samples
    //             sem_post(&four_slot_buffer->mutex);//Release all the semaphores 
    //             sem_post(&four_slot_buffer->empty_slots);
    //             break;
    //         }
    //         char *result = strdup(four_slot_buffer->data[four_slot_buffer->out]);
    //         //Copy the sample it reads into our reconstructed samples array
    //         //strcpy(reconstructed_samples[num_inputs],result);
    //         //printf("Consumer consumes [%d] %s\n",num_inputs,result);
    //         reconstructed_samples[num_samples] = result;
    //         num_samples++;
    //         //usleep(10000);
    //         four_slot_buffer->out = (four_slot_buffer->out + 1) % 4;//Increment the pointer to to point to the next slot to read a sample
    //         sem_post(&four_slot_buffer->mutex);// Release the mutex semaphore to allow other processes to access the shared buffer
    //         sem_post(&four_slot_buffer->empty_slots);//Signal the empty_slots semaphore to indicate that a slot has been consumed and is now empty
    //     } else {
    //         // Semaphore not available within the timeout, continue execution
    //         continue;
    //     }
    // }

    // Destroy the semaphores
    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);
    if (shmdt(four_slot_buffer) == -1) {
    perror("shmdt");
    exit(1);
    }
}
int main(int argc, char *argv[]) {
    int buffer_size = atoi(argv[1]);
    int argn = atoi(argv[2]);
    char *buffer_option = argv[3];
    int shm_id = atoi(argv[4]);
    int shm_id_2 = atoi(argv[5]);
    if (strcmp(buffer_option,"sync") == 0){
        sync_plot(buffer_size,argn,shm_id_2);
    }else{
        async_plot(argn,shm_id_2);
    }


    return 0;
}