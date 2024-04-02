#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    int is_sync = 0;
    int buffer_size = 1;
    char *test;
    char *program1 = NULL, *program2 = NULL, *program3 = NULL, *arg3 = NULL;
    //printf("Hellow");
    int opt;
    while((opt = getopt(argc,argv, "p:b:s:")) != -1){
        switch (opt){
            case 'p':
                if (optarg[0] == '1'){
                    printf("%c",optarg[0]);
                    program1 = optarg + 3;
                }else if (optarg[0] == '2'){
                    printf("%c",optarg[0]);
                    program2 = optarg + 3;
                }else if (optarg[0] == '3'){
                    printf("%c",optarg[0]);
                    program3 = optarg + 3;
                }
                break;
            case 'b':
                if (strcmp(optarg,"sync") == 0){
                    is_sync = 1;
                }
                break;
            case 's':
                buffer_size = atoi(optarg);
                break;

            default:
                exit(EXIT_FAILURE);
        }
    }
    printf("tapper -p1 %s -p2 %s -p3 %s -b %d -s %d", program1,program2,program3,is_sync,buffer_size);
    return 0;
}