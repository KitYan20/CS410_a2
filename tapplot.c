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
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    } RingBuffer;
    int num_samples = 0;//Keep track of the number of samples read
    char samples[MAX_SAMPLES][MAX_SAMPLES];//Initialize a samples array
    //Attach the ring buffer from the shared memory region specified by shm_id_2
    RingBuffer *ring_buffer = (RingBuffer*) shmat(shm_id_2, NULL, 0);
    if (ring_buffer == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    // Consumer
    while (1) {
        while(ring_buffer->in == ring_buffer->out){
            if (ring_buffer->done){//Finished reading all the data in the slots
                break;
            }
        };//wait if buffer is empty
        if (ring_buffer->done && ring_buffer->in == ring_buffer->out) {
            break;
        }
        //printf("Consumer 2 consumes %s\n", ring_buffer->data[ring_buffer->out]);
        strcpy(samples[num_samples], ring_buffer->data[ring_buffer->out]);//Read the sample from a slot in the Ring Buffer specified by 'out'
        num_samples++;//Increment the number of samples it has read
        ring_buffer->out = (ring_buffer->out + 1) % buffer_size;  // Move on to the next slot in the buffer using modulo
    }
    
    // Open a pipe to Gnuplot
    FILE *gnuplot_pipe = popen("gnuplot -persist", "w");
    if (gnuplot_pipe == NULL) {
        perror("popen");
        exit(1);
    }
     // Set up Gnuplot
    fprintf(gnuplot_pipe, "set title 'Plot'\n");
    fprintf(gnuplot_pipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplot_pipe, "set ylabel 'Value'\n");

    // Write the data to a temporary file
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        perror("tmpfile");
        exit(1);
    }

    for (int i = 0; i < num_samples; i++) {
        char *sample = samples[i];//Store a sample in a temporary sample string
        char *token = strtok(sample, ",");//Separate each value in the reconstructed samples via ','
        int field_count = 1;
        int value = 0;
        while (token != NULL) {
            if (field_count == argn) {
                char *value_str = strchr(token, '=');//Get the value for each name-value pair
                if (value_str != NULL) {//Skipe '='
                    value_str++; //Get the value for each name-value pair by incrementing the pointer to the next value stringand get the value for each name-value pair by incrementing the pointer to the next value string
                    value = atoi(value_str);//Convert the value to a integer 
                    break;//move on to the next sample to porcess
                }
            }
            token = strtok(NULL, ",");//Move on to the next value in a sample to plot if the field count doesn't match argn
            field_count++;//Increment the field count by one to check again
        }
        fprintf(temp_file, "%d %d\n", i + 1, value);//write the sample number and value into a temporary file
        fprintf(stdout,"Sample Number:%d - Value:%d\n", i + 1, value);//write the sample number and sample value plotted by tapplot back to stdout stream
    }
    //Rewind will be used to reset the file position indicator to the beginning of the temp file to read the constructed samples 
    //from the beginning of the file 
    rewind(temp_file);
    // Plot the data using Gnuplot
    fprintf(gnuplot_pipe, "plot '-' with linespoints\n");
    char line[100];//Initialize a line array to store each line from the temporary file
    while (fgets(line, sizeof(line), temp_file) != NULL) {//read each line from the temporary file
        fprintf(gnuplot_pipe, "%s", line);//
    }
    fprintf(gnuplot_pipe, "e\n");
    fflush(gnuplot_pipe);//Flush all the data to the gnuplot_pipe 

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
    //Four_Slot_Buffer *four_slot_buffer = (Four_Slot_Buffer*)shmat(shm_id_2, NULL, 0);
    int num_inputs = 0;
    char *reconstructed_samples[MAX_SAMPLES];
    // if (four_slot_buffer == (void*)-1) {
    //     perror("shmat");
    //     exit(1);
    // }
    
    FILE *gnuplot_pipe = popen("gnuplot -persist", "w");
    if (gnuplot_pipe == NULL) {
        perror("Error opening gnuplot pipe");
        exit(1);
    }

    fprintf(gnuplot_pipe, "set title 'Plot'\n");
    fprintf(gnuplot_pipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplot_pipe, "set ylabel 'Value'\n");
    fprintf(gnuplot_pipe, "plot 'reconstructed.txt' with lines\n");

    pclose(gnuplot_pipe);
    // if (shmdt(four_slot_buffer) == -1) {
    // perror("shmdt");
    // exit(1);
    // }
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