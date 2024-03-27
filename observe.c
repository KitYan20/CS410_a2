#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 256
#define MAX_NAMES 256
#define MAX_VALUES 256

void observe(FILE *input, void *buffer, int is_ring_buffer, int buffer_size){
    char line[MAX_LINE_LEN];
    char *names[MAX_NAMES];
    char *values[MAX_VALUES];
    int num_names = 0;
    int num_values = 0;
    char *end_name = NULL;

    while (fgets(line, MAX_LINE_LEN, stdin)) {
        char *name = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        int is_new_name = 1;
        for (int i = 0; i < num_names; i++) {
            if (strcmp(name, names[i]) == 0 && strcmp(value,values[i]) == 0) {
                is_new_name = 0;
                if (end_name == NULL) {
                    end_name = names[i];
                }
                break;
            }
        }
        if (is_new_name) {
            names[num_names++] = strdup(name);
            values[num_values++] = strdup(value);
            int len = strlen(line);
            write_to_buffer(buffer, line, len, is_ring_buffer, buffer_size);
            //printf("%s=%s\n", name, value);
            fflush(stdout);
        } else if (strcmp(name, end_name) == 0) {
            int len = strlen(line);
            write_to_buffer(buffer, line, len, is_ring_buffer, buffer_size);
            //printf("%s=%s\n", name, value);
            fflush(stdout);
        }

        // for (int i; i<num_names; i++){
        //     printf("%s=%s, " , names[i],values[i]);
        // }       
    }


}