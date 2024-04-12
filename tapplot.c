// tapplot.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_VALUE_SIZE 1064
#define MAX_SAMPLES 1064
typedef struct {
    char data[10][MAX_VALUE_SIZE];
    int in;
    int out;
    int done;
} RingBuffer;

int main(int argc, char *argv[]) {
    
    int buffer_size = atoi(argv[1]);
    int argn = atoi(argv[2]);
    char *buffer_option = argv[3];
    int shm_id = atoi(argv[4]);
    int shm_id_2 = atoi(argv[5]);

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

        //printf("Consumer 2 consumes %s\n", ring_buffer->data[ring_buffer->out]);
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
        double value = 0.0;

        while (token != NULL) {
            if (field_count == argn) {
                char *value_str = strchr(token, '=');
                if (value_str != NULL) {
                    value_str++;  // Skip '='
                    value = atof(value_str);
                    break;
                }
            }
            token = strtok(NULL, ",");
            field_count++;
        }

        fprintf(temp_file, "%d %f\n", i + 1, value);
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
    

    return 0;
}