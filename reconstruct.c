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
#define BUFFER_SIZE 10
#define MAX_VALUE_SIZE 1064
#define MAX_SAMPLES 1064
#define MAX_NAMES 256

// typedef struct {
//     char data[BUFFER_SIZE][MAX_VALUE_SIZE];
//     int in;
//     int out;
//     int done;
// } RingBuffer;
typedef struct {
    int sample_number;
    char value[MAX_NAMES];
} Sample;


void sync_reconstruct(int buffer_size,int argn,int shm_id,int shm_id_2){
    Sample samples[MAX_SAMPLES];
    char *meme[MAX_SAMPLES];
    int num_inputs = 0;
    char unique_names[MAX_NAMES][MAX_NAMES];
    int num_unique_names = 0;
    char end_name[MAX_NAMES];
    int current_sample = 0;
    typedef struct {
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
        
        if (ring_buffer->done && ring_buffer->in == ring_buffer->out){//Finished reading all the data in the slots
            // printf("Break\n");
            break;
        }
        char *result = strdup(ring_buffer->data[ring_buffer->out]);
        //printf("%s\n",meme[num_inputs]);
        meme[num_inputs] = result;
        
        ring_buffer->out = (ring_buffer->out + 1) % buffer_size;//Move on the next name value pair to read
        num_inputs++;
        
        //sleep(2); 
    }
    printf("\n");
    
    // for (int i = 0; i < num_inputs; i++){
    //     printf("[%d] %s\n",i,meme[i]);
    // }
    //printf("Num inputs %d\n",num_inputs);
    for (int i = 0; i < num_inputs; i++){
        char *comma = strchr(meme[i],',');
        if (comma != NULL){
            *comma = '\0';
        }
        //printf("%s\n",meme[i]);
    }
    for (int i = 0; i < num_inputs; i++){
        int is_new_name = 1;
        char *token = strdup(meme[i]);
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
        char *token = meme[i];
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
    // printf("Reconstruct Process\n");
    // printf("Observe to Reconstruct Consumer ID 1 %d\n",shm_id);
    // printf("Reconstruct to Tapplot Producer ID 2 %d\n",shm_id_2);
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
    //strcpy(ring_buffer_2->data[ring_buffer_2->in],samples[i].value);
    //printf("Producer produces %s\n",ring_buffer_2->data[ring_buffer_2->in]);
    while(i < current_sample){
         while((ring_buffer_2->in + 1) % buffer_size == ring_buffer_2->out){
             sleep(2);
         };// Wait if the buffer is full
    
         strcpy(ring_buffer_2->data[ring_buffer_2->in],samples[i].value);
         printf("Producer 2 produces %s\n",ring_buffer_2->data[ring_buffer_2->in]);
         i++;
         ring_buffer_2->in = (ring_buffer_2->in + 1) % buffer_size;//Move on to the next slot in the buffer using modulo
         //sleep(1);
         
    }
    ring_buffer_2->done = 1;//Ring buffer is done processing

    // FILE *gnuplot_file = fopen("data.txt", "w");
    // if (gnuplot_file == NULL) {
    //     perror("Error opening file");
    //     exit(1);
    // }
    // for (int i = 0; i < current_sample; i++) {
    //     printf("sample number %d, %s\n", samples[i].sample_number, samples[i].value);
    //     char *token = strtok(samples[i].value, ",");
    //     int field_count = 1;
    //     while (token != NULL) {
    //         if (field_count == argn) {
    //             char *value = strchr(token, '=');
    //             if (value != NULL) {
    //                 value++;
    //                 fprintf(gnuplot_file, "%d %s\n", samples[i].sample_number, value);
    //             }
    //             break;
    //         }
    //         token = strtok(NULL, ",");
    //         field_count++;
    //     }
        
    // }
    // fclose(gnuplot_file);

    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
    if (shmdt(ring_buffer_2) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void async_reconstruct(){
    Sample samples[MAX_SAMPLES];
    char *input_samples[MAX_SAMPLES];
    char *meme[MAX_SAMPLES];
    int num_inputs = 0;
    char unique_names[MAX_NAMES][MAX_NAMES];
    int num_unique_names = 0;
    char end_name[MAX_NAMES];
    int current_sample = 0;
    int i, j;
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
    while(!four_slot_buffer->done){
        sem_wait(&four_slot_buffer->full_slots);
        sem_wait(&four_slot_buffer->mutex);
        if (four_slot_buffer->done){
            printf("Consumer produce %s\n", four_slot_buffer->data[four_slot_buffer->out]);
            sem_post(&four_slot_buffer->mutex);
            sem_post(&four_slot_buffer->empty_slots);
            break;

        }
        printf("Consumer produce %s\n", four_slot_buffer->data[four_slot_buffer->out]);
        meme[num_inputs] = four_slot_buffer->data[four_slot_buffer->out];
        //input_samples[num_inputs] = ring_buffer->data[ring_buffer->out];
        // printf("%s\n",meme[num_inputs]);
        // printf("%d\n",num_inputs);
        num_inputs++;
        four_slot_buffer->out = (four_slot_buffer->out + 1) % 4;
        sleep(2);
        sem_post(&four_slot_buffer->mutex);
        sem_post(&four_slot_buffer->empty_slots);


        
    }

    printf("Done \n");
    for (i = 0; i < num_inputs; i++){
        printf("%s\n",meme[i]);
    }
    printf("Num inputs %d\n",num_inputs);
    for (int i = 0; i < num_inputs; i++){
        char *comma = strchr(meme[i],',');
        if (comma != NULL){
            *comma = '\0';
        }
        //printf("%s\n",meme[i]);
    }
    for (int i = 0; i < num_inputs; i++){
        int is_new_name = 1;
        char *token = strdup(meme[i]);
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");

        for (j = 0; j < num_unique_names; j++) {
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
    printf("%d\n",num_unique_names);
    // loop through all input data tokens to fill samples[] 
    for (int i = 0; i < num_inputs; i++) {
        char *token = meme[i];
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");
        // now we have name and value
        // if this sample's name is the end name, wrap up this sample 
        if (strcmp(name, end_name) == 0) {
            for (j = 0; j < num_unique_names - 1; j++) {
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
        for (j = 0; j < num_unique_names; j++) {
            if (strcmp(name, unique_names[j]) == 0) {
                strcpy(prev_values[j],value);
                break;
            }
        }

    }
    //printf("curr %d\n",current_sample);
    for (i = 0; i < current_sample; i++) {
        printf("sample number %d, %s\n", samples[i].sample_number, samples[i].value);
    }
    //Detach shared memory segment
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
    // printf("%d",argn);
    
    if (strcmp(buffer_option,"sync") == 0){
        sync_reconstruct(buffer_size,argn,shm_id,shm_id_2);
    }else{
        printf("%s\n",buffer_option);

        //async_reconstruct();
    }
    return 0;
}
