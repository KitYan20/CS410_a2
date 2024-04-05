#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX_SAMPLES 1064
#define MAX_NAMES 256
#define MAX_LINES 256


// Initialize a struct to store our samples
typedef struct {
    int sample_number;
    char value[MAX_NAMES];
} Sample;

//The text input have white space when I split each token with the "," delimeter
char *trim_leading_space(char *str){
    while(isspace(*str)){
        str++;
    }
    return str;
}
int main() {
    Sample samples[MAX_SAMPLES];
    char input[MAX_LINES];
    char* meme[MAX_SAMPLES];
    char* input_samples[MAX_SAMPLES];
    int num_inputs = 0;
    char unique_names[MAX_NAMES][MAX_NAMES];
    int num_unique_names = 0;
    char end_name[MAX_NAMES];
    int current_sample = 0;
    int i, j;

    fgets(input, sizeof(input), stdin);
    char* token = strtok(input, ",");
    
    while (token != NULL) {
        
        meme[num_inputs] = strdup(token);
        input_samples[num_inputs] = strdup(token);
        
        num_inputs++;
        token = strtok(NULL, ",");
    }

    printf("Number of inputs: %d\n", num_inputs);

    for (i = 0; i < num_inputs; i++) {
        char* name = strtok(trim_leading_space(meme[i]), "=");
        
        int is_new_name = 1;

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
                return 1;
            }
        } else {//Gets the last name from observe program
            strcpy(end_name, name);
        }
    }
    for (int i = 0; i < num_unique_names ; i++){
        printf("%s\n",unique_names[i]);
    }

    printf("Number of unique names: %d\n", num_unique_names);
    printf("End name: %s\n", end_name);

    char prev_values[MAX_NAMES][MAX_NAMES] = {0};

    for (i = 0; i < num_inputs; i++) {
        char *name = strtok(trim_leading_space(input_samples[i]), "=");
        char *value = strtok(NULL, "=");
        
        if (strcmp(name, end_name) == 0) {
            samples[current_sample].sample_number = current_sample + 1;            
            
            //printf("%s\n",samples[current_sample].value);
            current_sample++;
        }

        int name_found = 0;
        for (j = 0; j < num_unique_names; j++) {
            if (strcmp(name, unique_names[j]) == 0) {
                name_found = 1;
                strcpy(prev_values[j],value);
                
                break;
            }
        }

        for (j = 0; j < num_unique_names; j++) {
            if (strlen(prev_values[j]) > 0) {
                if (strlen(samples[current_sample].value) > 0) {
                    strcat(samples[current_sample].value, ", ");
                }
                strcat(samples[current_sample].value, unique_names[j]);
                strcat(samples[current_sample].value, "=");
                strcat(samples[current_sample].value, prev_values[j]);
            }
            
        }


    }


    for (i = 0; i < current_sample; i++) {
        printf("sample number %d ,%s\n", samples[i].sample_number, samples[i].value);
    }



    return 0;
}