#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_PROGRAMS 4
#define SHARED_MEM_SIZE 1024

typedef struct {
    char *name;
    char *arg;
} Programs;


int main(int argc, char *argv[]){
    int is_sync = 0;
    int buffer_size = 1;
    char *test;
    char *program1 = NULL, *program2 = NULL, *program3 = NULL, *arg3 = NULL;
    int optional;
    int opt;
    Programs programs[MAX_PROGRAMS];
    int num_programs = 0;
    while((opt = getopt(argc,argv, "p:b:s:o:")) != -1){
        switch (opt){
            case 'p':
                // if (optarg[0] == '1'){
                //     printf("%c",optarg[0]);
                //     program1 = optarg + 2;
                // }else if (optarg[0] == '2'){
                //     printf("%c",optarg[0]);
                //     program2 = optarg + 2;
                // }else if (optarg[0] == '3'){
                //     printf("%c",optarg[0]);
                //     program3 = optarg + 2;
                // }
                if (num_programs < MAX_PROGRAMS){
                    programs[num_programs].name= optarg + 2;
                    programs[num_programs].arg = NULL;
                
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
            case 'o':
                optional = atoi(optarg);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    //printf("tapper -p1 %s -p2 %s -p3 %s -o %d -b %d -s %d", program1,program2,program3,optional,is_sync,buffer_size);

        //shm_open creates and opens a new, or opens an existing, POSIX shared memory object.
    int shm_fd = shm_open("/observe_shm", O_CREAT | O_RDWR , 0666);

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
    //fork a child process for observe
    pid_t pid = fork();
    //The execl() function replaces the current process image with a new process image specified by path
    if (pid == 0){  
        execl(programs[0].name,programs[0].name,programs[0].arg,NULL);
        perror("execl");
        exit(EXIT_FAILURE);

    }else if (pid < 0){
        fprintf(stderr,"Error forking");
        exit(EXIT_FAILURE);
    }

    wait(NULL);

    char *shared_mem_ptr = shared_mem;
    while(*shared_mem_ptr != '\0'){
        printf("%s ",shared_mem_ptr);
        shared_mem_ptr += strlen(shared_mem_ptr)+1;
    }
    munmap(shared_mem,SHARED_MEM_SIZE);
    close(shm_fd);
    shm_unlink("/observe_shm");
    return 0;
}