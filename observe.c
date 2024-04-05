#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 1064
#define MAX_NAMES 100
#define SHARED_MEM_SIZE 1064

//Initialize a struct data type to store our name_value object
typedef struct {
    char name_value[MAX_VALUE_LEN];
} NameValue;

int main() {
    char input[256];//Initialize an array of chars to store our line of inputs from stdin or a file
    NameValue unique_names[MAX_NAMES];//create our NameValue struct data type object with size up to 100 names
    int num_unique_names = 0;//This will be our counter to count how many name_value pairs are stored 
    char result[256];//A result string to combine both the name and value of a input line

    //shm_open creates and opens a new, or opens an existing, POSIX shared memory object.
    int shm_fd = shm_open("/observe_shm", O_CREAT | O_RDWR ,0666);

    if (shm_fd == -1){
        printf("Error opening a shared memory object");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, SHARED_MEM_SIZE);//Truncate the shm_fd to be truncated to a size of precisely length bytes.
    char *shared_mem = mmap(NULL,SHARED_MEM_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED,shm_fd,0);
    if (shared_mem == MAP_FAILED){
        printf("Error creating a new mapping in the virtual address space");
        exit(EXIT_FAILURE);
    }
    while (fgets(input, sizeof(input), stdin) != NULL) {//Reading one line at a time from stdin
        char* equals_sign = strchr(input, '=');//Gets the value after "=" character
        if (equals_sign != NULL) {
            *equals_sign = '\0';  // Null-terminate the value
            char* name = input; //Get the name before "=" sign
            char* value = equals_sign + 1; //Get the value after the "=" sign
            char* newline = strchr(value, '\n');// Remove the newline from value
            if (newline != NULL) {
                *newline = '\0';//End it with a null terminated character
            }
            strcpy(result,name);//copy the name into the results string
            strcat(result,"=");//concatenate the string with "="
            strcat(result,value);//concatenate the string after "=" with the value
            
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
            }
            memset(result,'\0',sizeof(result));
        }
    }
    // for (int i = 0; i < num_unique_names; i++){
    //     printf("%s ", unique_names[i].name_value);

    // }
    char *shared_mem_ptr = shared_mem;
    for (int i = 0; i < num_unique_names; i++){
        strcpy(shared_mem_ptr,unique_names[i].name_value);
        shared_mem_ptr += strlen(unique_names[i].name_value) + 1;
        //printf("%s ", unique_names[i].name_value);

    }

    //unmapped and close the shared memory region
    munmap(shared_mem,SHARED_MEM_SIZE);
    close(shm_fd);
    return 0;
}