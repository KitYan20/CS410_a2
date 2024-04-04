#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_SAMPLES 1064
#define MAX_NAMES 256
#define MAX_LINES 256

// Initialize a struct to store our samples
typedef struct {
    int sample_number;
    char value[MAX_NAMES];
} Sample;

int main() {

    // initialize needed variables
    Sample samples[MAX_SAMPLES];
    char input[MAX_LINES];
    int num_inputs = 0;

    // stores input lines
    char* meme[MAX_SAMPLES];
    
    int num_unique_names = 0;
    char unique_names[MAX_NAMES][MAX_NAMES];
    char end_name[MAX_NAMES];
    int current_sample = 0;

    // for loops
    int i, j;

    // Reads the input lines from stdin and puts it in meme
    while (fgets(input, sizeof(input), stdin) != NULL) {
        char* token = strtok(input, ",");
        while (token != NULL) {
            // store token in meme
            meme[num_inputs] = strdup(token);
            num_inputs++;
            // get the next token from input
            token = strtok(NULL, ",");
        }
    }

    // Finds the end name and the number of unique names
    for (i = 0; i < num_inputs; i++) {

        //splits it using "=" as delimiter 
        char* name = strtok(meme[i], "=");
        int is_new_name = 1;

        for (j = 0; j < num_unique_names; j++) {
            if (strcmp(unique_names[j], name) == 0) {
                is_new_name = 0;
                break;
            }
        }

        if (is_new_name) {
            // copy new name to unique_names list
            strcpy(unique_names[num_unique_names], name);
            num_unique_names++;

            // error handling for edge case of too many names
            if (num_unique_names == MAX_NAMES) {
                fprintf(stderr, "Error: Maximum number of unique names exceeded\n");
                return 1;
            }
        } else {
            // If the current name is not new, it is the end name
            strcpy(end_name, name);
        }
    }

    // Reconstruct the samples
    for (i = 0; i < num_inputs; i++) {
        char* name = strtok(meme[i], "=");
        char* value = strtok(NULL, "=");

        if (strcmp(name, end_name) == 0) {
            // if current name is the end name, go to next sample
            current_sample++;

            // set sample number to next one
            samples[current_sample].sample_number = current_sample;
        }

        for (j = 0; j < num_unique_names; j++) {
            // check if current name is one of the unique names in list
            if (strcmp(unique_names[j], name) == 0) {
                // if it matches, copy the value to the current sample
                strcpy(samples[current_sample].value, value);
                // match found, exit now
                break;
            }
        }
    }

    // Prints out the reconstructed samples
    for (i = 1; i <= current_sample; i++) {
        printf("Sample #%d: ", samples[i].sample_number);
        for (j = 0; j < num_unique_names; j++) {
            printf("%s=%s", unique_names[j], samples[i].value);
            if (j != num_unique_names - 1) {
                printf(", ");
            }
        }
        printf("\n");
    }

    // Free memory
    for (i = 0; i < num_inputs; i++) {
        free(meme[i]);
    }

    return 0;
}