#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <semaphore.h>


#define MAX_NAMES 100
#define MAX_VALUE_SIZE 1064

typedef struct {
    char data[4][MAX_VALUE_SIZE];
    int in;
    int out;
    sem_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
    int done;
} Four_Slot_Buffer;

void sync_observe(int buffer_size,int shm_id){
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;

    char input[256];//Initialize an array of chars to store our line of inputs from stdin or a file
    char unique_names[MAX_NAMES][MAX_VALUE_SIZE];//Initialize an array of unique names strings
    int num_unique_names = 0;//Count the number of unique names the file
    char end_name[MAX_VALUE_SIZE] = "";
    char prev_values[MAX_NAMES][MAX_VALUE_SIZE] = {0};//Keep track of all the values it has encountered in the file
    int num_inputs = 0;
    char samples[MAX_VALUE_SIZE][MAX_VALUE_SIZE];
    /* Attach shared memory segment to shared_data */
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id,NULL,0);
    if (ring_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
    ring_buffer->in = 0;
    ring_buffer->out = 0;
    ring_buffer->done = 0;
    while (fgets(input, sizeof(input), stdin) != NULL) {//Reading one line at a time from stdin
        char* equals_sign = strchr(input, '=');//Gets the value after "=" character
        if (equals_sign != NULL) {
            *equals_sign = '\0';  // Null-terminate the value
            char* name = input; //Get the name before "=" sign
            char* value = equals_sign + 1; //Get the value after the "=" sign
            // Find the first occurrence of newline character
            value[strlen(value)-2] = ',';//Uncomment To test cs410-test-file
            char* newline = strchr(value, '\n');// Remove the newline from value
            if (newline != NULL) {
                *newline = '\0';//End it with a null terminated character
            }            
            int existing_name_index = -1;//initialize this variable to check if duplicate names appeared more than once
            for (int i = 0 ; i < num_unique_names; i++){//Iterate through the list of name_value pairs in unique_names
                if (strcmp(unique_names[i],name) == 0){//If the name_value already exist, we can break
                    existing_name_index = i;//Change the value to 1 to indicate the name already appeared once
                    break;
                }
            }
            if (existing_name_index == -1) {//Its a new name
                strcpy(unique_names[num_unique_names], name);//Copy the name into the unique names array
                num_unique_names++;//Increment the number of unique names
            }
            //If name does not exist
            if (strcmp(prev_values[existing_name_index], value) != 0){
                strcpy(prev_values[existing_name_index], value);
                sprintf(samples[num_inputs],"%s=%s", name, value);
                num_inputs++;
            }   
        }
    }
    for (int i ; i < num_inputs ; i++){ //<--- Use this to test out tappet output of observe
        printf("%s\n",samples[i]);
    }
    int i = 0;
    while (i < num_inputs){
        // printf("%s\n",samples[i]);
        while((ring_buffer->in + 1) % buffer_size == ring_buffer->out);
        // // Buffer is full, wait for space to become available
        strcpy(ring_buffer->data[ring_buffer->in],samples[i]);
        // // printf("Producer produced [%d] %s\n",i, ring_buffer->data[ring_buffer->in]);
        ring_buffer->in = (ring_buffer->in + 1) % buffer_size;//Move on to the next slot in the buffer using modulo
        i++;
    }
    // Continue processing until all input is consumed
    while (ring_buffer->in != ring_buffer->out) {
         sleep(1);
    }
    ring_buffer->done = 1;//Ring buffer is done processing
    
}

void async_observe(int shm_id) {
    //Attach the four slot buffer to a shared memory region
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*)shmat(shm_id, NULL, 0);
    if (four_slot_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    //initialize four-slot buffer
    four_slot_buffer->in = 0;
    four_slot_buffer->out = 0;
    four_slot_buffer->done = 0;
    //Initialize the sempahores with sem_init, every sccond argument of each semaphores is 1 to indicate the semaphore is shared between processes
    //The mutex semaphore prevents other processes (e.g., the consumer) from accessing the buffer simultaneously
    //the mutex semaphore is used as a binary semaphore (a mutex lock) to ensure mutual exclusion and prevent concurrent access to the shared buffer.
    sem_init(&four_slot_buffer->mutex,1,1);//set the mutex initial value to 1
    //The empty_slots semaphore is used to keep track of the number of empty slots in the shared buffer. Initially, all 4 slots are empty, so the semaphore is initialized with a value of 4.
    sem_init(&four_slot_buffer->empty_slots,1,4);//set the empty slots initial value to 4 to keep track of the number of empty slots in the shared buffer
    //The full_slots semaphore is used to keep track of the number of filled slots in the shared buffer. Initially, there are no filled slots, so the semaphore is initialized with a value of 0.
    sem_init(&four_slot_buffer->full_slots,1,0);//set the full slots initial value to 0 since there are no filled slots in the buffer

    char input[256];//Store the input from each line the file
    char unique_names[MAX_NAMES][MAX_VALUE_SIZE];//Initialize an array of unique names strings
    int num_unique_names = 0;//Count the number of unique names the file
    char end_name[MAX_VALUE_SIZE] = "";
    char prev_values[MAX_NAMES][MAX_VALUE_SIZE] = {0};//Keep track of all the values it has encountered in the file

    //struct timespec timeout;//Create a timespec timeout structure

    while (fgets(input, sizeof(input), stdin) != NULL) {//Read each line in a file
        char* equals_sign = strchr(input, '=');
        if (equals_sign != NULL) {
            *equals_sign = '\0';//End the name value will a null terminated character
            char* name = input;//Get the name string
            char* value = equals_sign + 1;//Increment the pointer to the value after the equal sign
            value[strlen(value)-2] = ',';//uncomment To test cs410-test-file
            char* newline = strchr(value, '\n');//Remove the new line
            if (newline != NULL) {
                *newline = '\0';//End it with a null terminated character
            }
            int existing_name_index = -1;//Initialize the existing name index to -1 to be used for sending the proper samples
            for (int i = 0; i < num_unique_names; i++) {
                //A for loop to check for all the unique names in the file
                //If the name exist, we set the index of the position where the names are identified
                if (strcmp(unique_names[i], name) == 0) {
                    existing_name_index = i;
                    break;
                }
            }
            if (existing_name_index == -1) {//Its a new name
                strcpy(unique_names[num_unique_names], name);//Copy the name into the unique names array
                num_unique_names++;//Increment the number of unique names
                // if (num_unique_names > 1) {
                //     strcpy(end_name, name);//get the last unique to construct a sample
                // }
            }
            if (strcmp(prev_values[existing_name_index], value) != 0) {
                //Main code to produce the required samples
                strcpy(prev_values[existing_name_index], value);
                //clock_gettime(CLOCK_REALTIME, &timeout);// Get the current time and store it in the 'timeout' structure
                //timeout.tv_sec += 1; // Set timeout to 1 second from the current time
                //It will attempt to acquire the 'empty_slots' semaphore within the specified timeout
                if (sem_trywait(&four_slot_buffer->empty_slots) == 0) {
                    sem_wait(&four_slot_buffer->mutex);//acquire a mutex lock to allow for write access to shared buffer
                    //Use sprintf to store the name-value pair in a specified format in a slot specified by "in" indice
                    sprintf(four_slot_buffer->data[four_slot_buffer->in], "%s=%s", name, value);
                    //printf("Producer produces %s\n", four_slot_buffer->data[four_slot_buffer->in]);
                    four_slot_buffer->in = (four_slot_buffer->in + 1) % 4;//Increment in to point to the next slot in the buffer
                    sem_post(&four_slot_buffer->mutex);//Release the mutex lock to allow another process to access the shared bufefer
                    sem_post(&four_slot_buffer->full_slots);//Signal to the full slots semaphore to indicate a new slot is filled
                    //sleep(1);
                } else {
                    // Semaphore not available within the timeout, continue execution
                    sem_wait(&four_slot_buffer->mutex);
                    if (four_slot_buffer->done){
                        sem_post(&four_slot_buffer->mutex);
                        break;
                    }
                    sem_post(&four_slot_buffer->mutex);
                    // Wait for a short duration using nano sleep before trying again
                    struct timespec wait_time;
                    wait_time.tv_sec = 0;
                    wait_time.tv_nsec = 10000000; // 10 milliseconds
                    nanosleep(&wait_time, NULL);
                }
            }
        }
    }
    four_slot_buffer->done = 1;//Indicate that the buffer has processed all the samples 
    sem_post(&four_slot_buffer->full_slots);//Release the full slots lock to allow for consumer to proceed
}

int main(int argc, char *argv[]) {
    int buffer_size = atoi(argv[1]);//get buffer size for synchronous buffering
    char *buffer_option = argv[3];//get the buffer option
    int shm_id = atoi(argv[4]);//get the shared memory id
    if (strcmp(buffer_option,"sync") == 0){
        sync_observe(buffer_size,shm_id);
    }else{
        //printf("Observe %s\n",buffer_option);
        async_observe(shm_id);
    }
    return 0;
}