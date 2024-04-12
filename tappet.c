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
#include <pthread.h>


#define MAX_PROGRAMS 100
#define SHM_SIZE 1024
#define MAX_VALUE_SIZE 1064
#define BUFFER_SIZE 10
#define MAX_THREADS 10
// void *print_message_function( void *ptr );

// int main()
// {
//      pthread_t thread1, thread2, thread3, thread4, thread5, thread6, thread7;
//      char *message1 = "Thread 1";
//      char *message2 = "Thread 2";
//      char *message3 = "Thread 3";
//      char *message4 = "Thread 4";
//      int  iret1, iret2, iret3, iret4;

//      iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
//      iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);
//      iret3 = pthread_create( &thread3, NULL, print_message_function, (void*) message3);
//      iret4 = pthread_create( &thread4, NULL, print_message_function, (void*) message4);

     
//     // Wait for all threads to complete
//     pthread_join(thread1, NULL);
//     pthread_join(thread2, NULL);
//     pthread_join(thread3, NULL);
//     pthread_join(thread4, NULL);
//     return 0;
// }

// void *print_message_function( void *ptr )
// {
//      char *message;
//      if(strcmp(ptr, "Thread 1") != 0){
// 	     sleep(3);
//      }
//      message = (char *) ptr;
//      printf("%s \n", message);
// }


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
    //Each thread corresponds to one program 

    //for (int )
    return 0;
}