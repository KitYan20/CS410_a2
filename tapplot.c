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
    int num_inputs = 0;
    char *reconstructed_samples[MAX_SAMPLES];
    if (four_slot_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    printf("Hello\n");
    //printf("Consumer consumed %s\n",four_slot_buffer->data[four_slot_buffer->out]);
    //struct timespec timeout;
    //Consumer
    //Code needs to be FIXED
    // Consumer
    while (1) {
        //sem_trywait attempts to acquire the semaphore immediately and returns if it is not available
        //If sem_trywait successfully acquires the empty_slots semaphore, the code proceeds as before, 
        //writing the name-value pair to the shared buffer.
        // Attempt to acquire the 'full_slots' semaphore using sem_trywait
        if (sem_trywait(&four_slot_buffer->full_slots) == 0) {
            // If the semaphore is acquired successfully
            // Attempting to acquire a mutex semaphore
            // If acquired, it'll gain exclusive access to the shared buffer
            sem_wait(&four_slot_buffer->mutex);
            // Check if the producer is done and the buffer is empty
            if (four_slot_buffer->done && four_slot_buffer->in == four_slot_buffer->out) {
                // Release all the semaphores
                sem_post(&four_slot_buffer->mutex);
                sem_post(&four_slot_buffer->empty_slots);
                break;
            }
            // Copy the sample it reads into our reconstructed samples array
            char *result = strdup(four_slot_buffer->data[four_slot_buffer->out]);
            reconstructed_samples[num_inputs] = result;
            num_inputs++;
            // Increment the pointer to point to the next slot to read a sample
            four_slot_buffer->out = (four_slot_buffer->out + 1) % 4;
            // Release the mutex semaphore to allow other processes to access the shared buffer
            sem_post(&four_slot_buffer->mutex);
            // Signal the empty_slots semaphore to indicate that a slot has been consumed and is now empty
            sem_post(&four_slot_buffer->empty_slots);
        //If sem_trywait fails to acquire the semaphore (i.e., returns a non-zero value), 
        //it means that there are no empty slots available in the buffer. 
        //In this case, the code checks if the consumer is done by acquiring the mutex semaphore and checking the done flag. 
        //If the consumer is done, the code releases the mutex semaphore and breaks out of the loop.
        // If the consumer is not done, the code releases the mutex semaphore and waits for a short duration (10 milliseconds) using nanosleep before trying again.   
        } else {
            // Semaphore not available, check if the producer is done
            sem_wait(&four_slot_buffer->mutex);
            if (four_slot_buffer->done) {
                sem_post(&four_slot_buffer->mutex);
                break;
            }
            sem_post(&four_slot_buffer->mutex);//Release the mutex semaphore to allow other processes to access shared buffer

            // Wait for a short duration using nanosleep before trying again
            struct timespec wait_time;
            wait_time.tv_sec = 0;
            wait_time.tv_nsec = 10000000; // 10 milliseconds
            nanosleep(&wait_time, NULL);
        }
    }

    // Destroy the semaphores
    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);

    for (int i = 0; i < num_inputs ; i++){
        printf("Consumer 2 consumed %s\n",reconstructed_samples[i]);
    }
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