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

char *trim_leading_space(char *str){
    while(isspace(*str)){
        str++;
    }
    return str;
}

void sync_reconstruct(int buffer_size){
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
    char result[256];
    while(1){
        // Read observed changes from the ring buffer
        while(ring_buffer->in == ring_buffer->out && !ring_buffer->done);//wait if buffer is empty
        
        if (ring_buffer->done && ring_buffer->in == ring_buffer->out){//Finished reading all the data in the slots
            printf("Break\n");
            break;
        }
        meme[num_inputs] = ring_buffer->data[ring_buffer->out];
        input_samples[num_inputs] = ring_buffer->data[ring_buffer->out];
        strcat(result,ring_buffer->data[ring_buffer->out]);
        num_inputs++;
        ring_buffer->out = (ring_buffer->out + 1) % BUFFER_SIZE;//Move on the next name value pair to read
        //sleep(2);
        
 
    }
    printf("\n");
    
    for (int i = 0; i < num_inputs; i++){
        char *comma = strchr(meme[i],',');
        
        
        if (comma != NULL){
            *comma = '\0';
        }
    
        // //printf("Comma_1=%s\n", meme[i]);
        // char *token = meme[i];
        // char* name = strtok(, "=");
        // char* value = strtok(NULL, "=");

        //printf("name=%s, v=%s\n", name, value );
        // strcat(result,name);
        // strcat(result,"=");
        // strcat(result,value);
        // input_samples[i] = result;
        // //printf("Consumer produce %s\n", input_samples[i]);
        // memset(result,'\0',sizeof(result));
    }
    for (int i = 0; i < num_inputs; i++){
        int is_new_name = 1;
        //printf("%s\n", meme[i]);
     
        char *token = strdup(meme[i]);
        //printf("token=%s\n",token);
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");
        //printf("name=%s,value=%s\n\n",name,value);
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
    
    // loop through all input data tokens to fill samples[] 
    for (int i = 0; i < num_inputs; i++) {
        char *token = meme[i];
        printf("token=%s\n",token);
        char *name = strtok(token, "=");
        char* value = strtok(NULL, "=");
        printf("name=%s, v=%s\n", name, value);
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


    for (i = 0; i < current_sample; i++) {
        printf("sample number %d, %s\n", samples[i].sample_number, samples[i].value);
    }


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
