#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>


#define MAX_PROGRAMS 100
#define SHM_SIZE 1024
#define MAX_VALUE_SIZE 1064
#define BUFFER_SIZE 10
typedef struct {
    char *name;
    int optional;
} Programs;

int main(int argc, char *argv[]){
    int is_sync = 0;
    int buffer_size = 1;
    int optional = 1;
    int opt;
    Programs programs[MAX_PROGRAMS];
    int num_programs = 0;
    while((opt = getopt(argc,argv, "p:b:s:o:")) != -1){
        switch (opt){
            case 'p':
                    programs[num_programs].name= optarg + 2;
                    num_programs++;
                break;
            case 'b':
                if (strcmp(optarg,"sync") == 0){
                    is_sync = 1;
                }
                break;
            case 's':
                buffer_size = atoi(optarg);
                break;
            case 'o':
                optional = atoi(optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }

    }
    printf("tappet");
    for (int i = 0 ; i < num_programs ; i++){
        printf(" -p%d %s",i+1, programs[i].name);
    }
    printf(" -o %d -b %d -s %d\n", optional,is_sync,buffer_size);

    return 0;
}