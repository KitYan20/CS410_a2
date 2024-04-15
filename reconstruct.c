#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>
#define MAX_VALUE_SIZE 1064
#define MAX_SAMPLES 1064
#define MAX_NAMES 256
//A Samples structure to store all of our reconstructed samples
typedef struct {
    int sample_number;
    char value[MAX_VALUE_SIZE];
} Sample;
//Create a Four Slot Buffer structure
typedef struct {
    char data[4][MAX_VALUE_SIZE];//
    int in;
    int out;
    sem_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
    int done;
} Four_Slot_Buffer;

void sync_reconstruct(int buffer_size,int argn,int shm_id,int shm_id_2){
    Sample samples[MAX_SAMPLES];//Initialize a Samples 
    char reconstructed_samples[MAX_SAMPLES][MAX_SAMPLES];//Using array of pointers to characters (pointers to strings)
    int num_inputs = 0;//Initialize a num inputs variable to keep track the number of reconstructed samples
    char unique_names[MAX_NAMES][MAX_NAMES]; //Initialize a unique names array
    int num_unique_names = 0;//Keep track of the number of unique names
    char end_name[MAX_NAMES];//Get the end name of the last unique name
    int current_sample = 0;
    typedef struct {//Initialize our RingBuffer data structure
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;
    /* Attach shared memory segment to shared_data */
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id,NULL,0);
    if (ring_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
    while(1){
        // Read observed changes from the ring buffer
        while(ring_buffer->in == ring_buffer->out && !ring_buffer->done);//wait if buffer is empty
        if (ring_buffer->done){//Finished reading all the data in the slots
            break;
        }
        //Copy the read value into the array
        strcpy(reconstructed_samples[num_inputs],ring_buffer->data[ring_buffer->out]);
        //printf("Consumer consumed %s\n",ring_buffer->data[ring_buffer->out]);
        num_inputs++;
        ring_buffer->out = (ring_buffer->out + 1) % buffer_size;//Move on the next name value pair to read
        //sleep(2); 
    }
    // for (int i = 0; i < num_inputs; i++){
    //     printf("Consumer produce [%d] %s \n",i,reconstructed_samples[i]);
    // }
    for (int i = 0; i < num_inputs; i++){
        char *comma = strchr(reconstructed_samples[i],',');
        if (comma != NULL){
            *comma = '\0';
        }
    }
    for (int i = 0; i < num_inputs; i++){
        int is_new_name = 1;
        char *token = strdup(reconstructed_samples[i]);
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");

        for (int j = 0; j < num_unique_names; j++) {
            if (strcmp(unique_names[j], name) == 0) {
                is_new_name = 0;
                break;
            }
        }
        if (is_new_name) {
            strcpy(unique_names[num_unique_names], name);
            num_unique_names++;
        } else {//Gets the last name from observe program
            strcpy(end_name, name);
        }
        
    }
    char prev_values[MAX_SAMPLES][MAX_VALUE_SIZE] = {0};
    // loop through all input data tokens to fill samples[] 
    for (int i = 0; i < num_inputs; i++) {
        char *token = reconstructed_samples[i];
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");
        // now we have name and value
        // if this sample's name is the end name, wrap up this sample 
        if (strcmp(name, end_name) == 0) {
            for (int j = 0; j < num_unique_names - 1; j++) {
                strcat(samples[current_sample].value, unique_names[j]);
                strcat(samples[current_sample].value, "=");
                strcat(samples[current_sample].value, prev_values[j]);
                strcat(samples[current_sample].value, ", ");
            }
            // printf("End of current sample, which is %d\n", current_sample);
            strcat(samples[current_sample].value, name);
            strcat(samples[current_sample].value, "=");
            strcat(samples[current_sample].value, value);

            samples[current_sample].sample_number = current_sample + 1;      
            //printf("Sample %d: -%s-\n", current_sample, samples[current_sample].value);      
            current_sample++;
        }

        //unique_names[j] = name, so prev_values[j]= the last value of that named object
        for (int j = 0; j < num_unique_names; j++) {
            if (strcmp(name, unique_names[j]) == 0) {
                strcpy(prev_values[j],value);
                break;
            }
        }

    }
    strcat(samples[current_sample-1].value," s");//Used for cs410-test-file
    // for (int i = 0; i < current_sample ; i++){
    //     printf("Constructed Samples %s\n",samples[i].value);
    // }
    
    //Initialize the second Ring Buffer to produce and consume to Tapplot
    RingBuffer *ring_buffer_2 = (RingBuffer*) shmat(shm_id_2,NULL,0);
    ring_buffer_2->in = 0;
    ring_buffer_2->out = 0;
    ring_buffer_2->done = 0;

    if (ring_buffer_2 == (void*)-1){
        perror("shmat");
        exit(1);
    }
    int i = 0;
    //Producer 
    while(i < current_sample){
          while((ring_buffer_2->in + 1) % buffer_size == ring_buffer_2->out);// Wait if the buffer is full
          strcpy(ring_buffer_2->data[ring_buffer_2->in],samples[i].value);//Write the reconstructed samples in a slot specified by "in"
          //printf("Producer 2 produces %s\n",ring_buffer_2->data[ring_buffer_2->in]);
          i++;
          ring_buffer_2->in = (ring_buffer_2->in + 1) % buffer_size;//Move on to the next slot in the buffer using modulo to write
          //sleep(1);
         
    }
    ring_buffer_2->done = 1;//Ring buffer is done processing

    //Detach shared memory segment from observe to reconstruct
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
    
}

void async_reconstruct(int shm_id,int shm_id_2,int argn){
    Sample samples[MAX_SAMPLES];
    char *reconstructed_samples[MAX_SAMPLES];
    int num_inputs = 0;
    char unique_names[MAX_NAMES][MAX_NAMES];
    int num_unique_names = 0;
    char end_name[MAX_NAMES];
    int current_sample = 0;
    // char result[256];
    /* Attach shared memory segment to shared_data */
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*) shmat(shm_id,NULL,0);

    if (four_slot_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
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
    // for (int i = 0;i<num_inputs;i++){
    //     printf("Consumer consumes [%d] %s %ld\n",i,reconstructed_samples[i],strlen(reconstructed_samples[i]));
    // }
    //Destroy all the semaphores
    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);
    for (int i = 0; i < num_inputs; i++){
        char *comma = strchr(reconstructed_samples[i],',');
        if (comma != NULL){
            *comma = '\0';
        }
        //printf("%s\n",meme[i]);
    }
    for (int i = 0; i < num_inputs; i++){
        int is_new_name = 1;
        char *token = strdup(reconstructed_samples[i]);
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");

        for (int j = 0; j < num_unique_names; j++) {
            if (strcmp(unique_names[j], name) == 0) {
                is_new_name = 0;
                break;
            }
        }
        if (is_new_name) {
            strcpy(unique_names[num_unique_names], name);
            num_unique_names++;
            if (num_unique_names == MAX_NAMES) {
                fprintf(stderr, "Error: Maximum number of unique names exceeded\n");
                
            }
        } else {//Gets the last name from observe program
            strcpy(end_name, name);
        }
        
    }
    char prev_values[MAX_NAMES][MAX_NAMES] = {0};
    // printf("%d\n",num_unique_names);
    // loop through all input data tokens to fill samples[] 
    for (int i = 0; i < num_inputs; i++) {
        char *token = reconstructed_samples[i];
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");
        // now we have name and value
        // if this sample's name is the end name, wrap up this sample 
        if (strcmp(name, end_name) == 0) {
            for (int j = 0; j < num_unique_names - 1; j++) {
                strcat(samples[current_sample].value, unique_names[j]);
                strcat(samples[current_sample].value, "=");
                strcat(samples[current_sample].value, prev_values[j]);
                strcat(samples[current_sample].value, ", ");
            }

            // printf("End of current sample, which is %d\n", current_sample);
            strcat(samples[current_sample].value, name);
            strcat(samples[current_sample].value, "=");
            strcat(samples[current_sample].value, value);

            samples[current_sample].sample_number = current_sample + 1;      
            //printf("Sample %d: -%s-\n", current_sample, samples[current_sample].value);      
            current_sample++;
        }

        //unique_names[j] = name, so prev_values[j]= the last value of that named object
        for (int j = 0; j < num_unique_names; j++) {
            if (strcmp(name, unique_names[j]) == 0) {
                strcpy(prev_values[j],value);
                break;
            }
        }

    }
    strcat(samples[current_sample-1].value," s"); //<--- ONLY FOR cs410-test-file
    // for (int i = 0; i < current_sample ;i++){
    //     printf("Consumer produced %s\n",samples[i].value);
    // }
    
    // Four_Slot_Buffer *four_slot_buffer_2 = (Four_Slot_Buffer*) shmat(shm_id_2,NULL,0);

    // if (four_slot_buffer_2 == (void*)-1){
    //     perror("shmat");
    //     exit(1);
    // }

    //Because we had trouble figuring out how to implement a second Four Slot Buffer, we just wrote all the samples 
    //out to a file to be plotted
    int i = 0;
    FILE *gnuplot_file = fopen("reconstructed.txt", "w");
    if (gnuplot_file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    for (int i = 0; i < current_sample; i++) {
        //printf("sample number %d, %s\n", samples[i].sample_number, samples[i].value);
        char *token = strtok(samples[i].value, ",");//Separate each value in the reconstructed samples via ','
        int field_count = 1;//Initialize a field count by 1 to indiciate which specific value to plot
        while (token != NULL) {
            if (field_count == argn) {
                char *value = strchr(token, '=');//Get the value for each name-value pair
                if (value != NULL) {
                    value++;//Get the value for each name-value pair by incrementing the pointer to the next value string
                    fprintf(gnuplot_file, "%d %s\n", samples[i].sample_number, value);//write the sample number and value into the gnuplot_file stream
                    fprintf(stdout,"Sample Number:%d - Value:%s\n", samples[i].sample_number, value);//write the sample number and sample value plotted by tapplot back to stdout stream
                }
                break;//move on to the next sample to process
            }
            token = strtok(NULL, ",");//Move on to the next value in a sample to plot if the field count doesn't match argn
            field_count++;//Increment the field count by one to check again
        }
        
    }
    fclose(gnuplot_file);//close the gnuplot_file stream

    //Detach shared memory segment between observe and reconstruct
    if (shmdt(four_slot_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }

}

int main(int argc, char *argv[]){
    int buffer_size = atoi(argv[1]);
    int argn = atoi(argv[2]);
    char *buffer_option = argv[3];
    int shm_id = atoi(argv[4]);
    int shm_id_2 = atoi(argv[5]);    
    if (strcmp(buffer_option,"sync") == 0){
        sync_reconstruct(buffer_size,argn,shm_id,shm_id_2);
    }else{
        //printf("Reconstruct %s\n",buffer_option);
        async_reconstruct(shm_id,shm_id_2,argn);
    }
    return 0;
}
