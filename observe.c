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

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 1064
#define MAX_NAMES 100

#define BUFFER_SIZE 10
#define MAX_VALUE_SIZE 1064

// typedef struct {
//     char data[BUFFER_SIZE][MAX_VALUE_SIZE];
//     int in;
//     int out;
//     int done;

// } RingBuffer;
typedef struct {
    char data[4][MAX_VALUE_SIZE];
    int in;
    int out;
    sem_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
    int done;
} Four_Slot_Buffer;

typedef struct {
    char name_value[MAX_VALUE_LEN];
} NameValue;

void sync_observe(int buffer_size,int shm_id){
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;

    } RingBuffer;

    char input[256];//Initialize an array of chars to store our line of inputs from stdin or a file
    NameValue unique_names[MAX_NAMES];//create our NameValue struct data type object with size up to 100 names
    int num_unique_names = 0;//This will be our counter to count how many name_value pairs are stored 
    char result[256];//A result string to combine both the name and value of a input line

    /* Attach shared memory segment to shared_data */
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id,NULL,0);

    if (ring_buffer == (void*)-1){
        perror("shmat");
        exit(1);
    }
    
    while (fgets(input, sizeof(input), stdin) != NULL) {//Reading one line at a time from stdin
        char* equals_sign = strchr(input, '=');//Gets the value after "=" character
        if (equals_sign != NULL) {
            *equals_sign = '\0';  // Null-terminate the value
            char* name = input; //Get the name before "=" sign
            char* value = equals_sign + 1; //Get the value after the "=" sign
            // Find the first occurrence of newline character
            //value[strlen(value)-2] = ',';//Uncomment To test cs410-test-file
            char* newline = strchr(value, '\n');// Remove the newline from value
            if (newline != NULL) {
                //*newline++ = ',';
                *newline = '\0';//End it with a null terminated character
            }
            strcpy(result,name);//copy the name into the results string
            strcat(result,"=");//concatenate the string with "="
            strcat(result,value);//concatenate the string after "=" with the value
            //value[strlen(value)] = ',';
            
            //printf("%s\n",result);
            int existing_name_index = 0;//initialize this variable to check if duplicate names appeared more than once
            for (int i = 0 ; i < num_unique_names; i++){//Iterate through the list of name_value pairs in unique_names
                if (strcmp(unique_names[i].name_value,result) == 0){//If the name_value already exist, we can break
                    existing_name_index = 1;//Change the value to 1 to indicate the name already appeared once
                    break;
                }
            }
            //If name does not exist
            if (existing_name_index == 0){
                strcpy(unique_names[num_unique_names].name_value,result);//Copy the result of the string into the corresponding indice into the struct
                num_unique_names++;//Increment to move on to the next position

                //printf("%s\n",ring_buffer->data[ring_buffer->write_index]);
                while((ring_buffer->in + 1) % buffer_size == ring_buffer->out){
                    // Buffer is full, wait for space to become available
                    sleep(1);
                };
                //Write the observe change to ring buffer
                strcpy(ring_buffer->data[ring_buffer->in],result);
                printf("Producer produces %s\n",ring_buffer->data[ring_buffer->in]);
                ring_buffer->in = (ring_buffer->in + 1) % buffer_size;//Move on to the next slot in the buffer using modulo
                //sleep(1);
                
            }
            memset(result,'\0',sizeof(result));
        }
    }
    // Continue processing until all input is consumed
    while (ring_buffer->in != ring_buffer->out) {
        sleep(1);
    }
    ring_buffer->done = 1;//Ring buffer is done processing
    //Detach shared memory segment
    if (shmdt(ring_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void async_observe(int shm_id) {
    Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*)shmat(shm_id, NULL, 0);
    if (four_slot_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    char input[256];
    char unique_names[MAX_NAMES][MAX_VALUE_LEN];
    int num_unique_names = 0;
    char end_name[MAX_VALUE_LEN] = "";
    char prev_values[MAX_NAMES][MAX_VALUE_LEN] = {0};

    struct timespec timeout;

    while (fgets(input, sizeof(input), stdin) != NULL) {
        char* equals_sign = strchr(input, '=');
        if (equals_sign != NULL) {
            *equals_sign = '\0';
            char* name = input;
            char* value = equals_sign + 1;
            value[strlen(value)-2] = ',';//uncomment To test cs410-test-file
            char* newline = strchr(value, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }

            int existing_name_index = -1;
            for (int i = 0; i < num_unique_names; i++) {
                if (strcmp(unique_names[i], name) == 0) {
                    existing_name_index = i;
                    break;
                }
            }

            if (existing_name_index == -1) {
                strcpy(unique_names[num_unique_names], name);
                num_unique_names++;
                if (num_unique_names > 1) {
                    strcpy(end_name, name);
                }
            }

            if (strcmp(prev_values[existing_name_index], value) != 0) {

                strcpy(prev_values[existing_name_index], value);

                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_sec += 1; // Set timeout to 1 second from now

                if (sem_timedwait(&four_slot_buffer->empty_slots, &timeout) == 0) {
                    sem_wait(&four_slot_buffer->mutex);
                    sprintf(four_slot_buffer->data[four_slot_buffer->in], "%s=%s", name, value);
                    //printf("Producer produces %s\n", four_slot_buffer->data[four_slot_buffer->in]);
                    four_slot_buffer->in = (four_slot_buffer->in + 1) % 4;
                    sem_post(&four_slot_buffer->mutex);
                    sem_post(&four_slot_buffer->full_slots);
                } else {
                    // Semaphore not available within the timeout, continue execution
                    continue;
                }
            }
        }
    }

    four_slot_buffer->done = 1;
    sem_post(&four_slot_buffer->full_slots);

    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);

    if (shmdt(four_slot_buffer) == -1) {
        perror("shmdt");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    //printf("Buffer size %d\n",atoi(argv[1]));
    int buffer_size = atoi(argv[1]);
    char *buffer_option = argv[3];
    int shm_id = atoi(argv[4]);
    //printf("Buffer option %s\n",buffer_option);
    if (strcmp(buffer_option,"sync") == 0){
        //printf("Observe Id %d\n",shm_id);
        sync_observe(buffer_size,shm_id);
    }else{
        printf("Observe %s\n",buffer_option);
        async_observe(shm_id);
    }
    return 0;
}