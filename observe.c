#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 1064
#define MAX_NAMES 100

typedef struct {
    char name_value[MAX_VALUE_LEN];
} NameValue;

int main() {
    char input[256];
    NameValue unique_names[MAX_NAMES];
    int num_unique_names = 0;
    int end_name_index = -1;
    char result[256];
    while (fgets(input, sizeof(input), stdin) != NULL) {
        char* equals_sign = strchr(input, '=');
        if (equals_sign != NULL) {
            *equals_sign = '\0';  // Null-terminate the name
            char* name = input;
            char* value = equals_sign + 1;
                        // Remove newline from value
            char* newline = strchr(value, '\n');
            if (newline != NULL) {
                *newline = '\0';

            }
            strcpy(result,name);
            strcat(result,"=");
            strcat(result,value);
            int existing_name_index = -1;
            for (int i = 0 ; i < num_unique_names; i++){
                if (strcmp(unique_names[i].name_value,result) == 0){
                    existing_name_index = i;
                    break;
                }
            }
            //If name does not exist
            if (existing_name_index == -1){
                strcpy(unique_names[num_unique_names].name_value,result);
                num_unique_names++;
                end_name_index = num_unique_names - 1;
            }else if (existing_name_index != -1 && existing_name_index == end_name_index){
                
            }
            memset(result,'\0',sizeof(result));
        }
    }
    for (int i = 0; i < num_unique_names; i++){
        printf("%s ", unique_names[i].name_value);

    }

    return 0;
}