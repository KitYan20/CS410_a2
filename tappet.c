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
#include <dlfcn.h>


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
typedef struct {
    int buffer_size;
    int shm_id;
    int shm_id2;
    int opt_arg;
    char buffer_option[256];
    pthread_mutex_t mutex;
    pthread_cond_t cond_observe;
    pthread_cond_t cond_reconstruct;
    int observe_done;
    int reconstruct_done;

} Args;
typedef struct {
    char data[4][MAX_VALUE_SIZE];
    int in;
    int out;
    sem_t mutex;
    sem_t empty_slots;
    sem_t full_slots;
    int done;
} Four_Slot_Buffer;
void *run_observe(void *arg){
    Args *args = (Args*)arg;
    int buffer_size = args->buffer_size;
    int shm_id = args->shm_id;
    char *buffer_option = args->buffer_option;
    //printf("%d %s\n",buffer_size,buffer_option);
    void *handle;
    handle = dlopen("./sharedlibrary.so",RTLD_LAZY);

    if (!handle){
        fprintf(stderr,"Error loading shared library %s\n", dlerror());
    }
    if (strcmp(buffer_option,"async") == 0){
        //printf("async function\n");
        void (*async_observe)(int) = dlsym(handle,"async_observe");
        if (!async_observe) {
            fprintf(stderr, "Error loading async_observe function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        async_observe(shm_id);

    }else{
        //printf("sync function\n");
        void (*sync_observe)(int,int) = dlsym(handle,"sync_observe");
        if (!sync_observe) {
            fprintf(stderr, "Error loading sync_observe function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        sync_observe(buffer_size,shm_id);
    }
    // pthread_mutex_lock(&args->mutex);
    // args->observe_done = 1;
    // pthread_cond_signal(&args->cond_observe);
    // pthread_mutex_unlock(&args->mutex);

    dlclose(handle);
    return NULL;
}

void *run_reconstruct(void *arg){
    Args *args = (Args*)arg;
    int buffer_size = args->buffer_size;
    int shm_id = args->shm_id;
    int shm_id_2 = args->shm_id2;
    int argn = args->opt_arg;
    char *buffer_option = args->buffer_option;

    pthread_mutex_lock(&args->mutex);
    while(!args->observe_done){
        pthread_cond_wait(&args->cond_observe, &args->mutex);
    }
    pthread_mutex_unlock(&args->mutex);
    void *handle;
    handle = dlopen("./sharedlibrary.so",RTLD_LAZY);

    if (!handle){
        fprintf(stderr,"Error loading shared library %s\n", dlerror());
    }
    if (strcmp(buffer_option,"async") == 0){
        //printf("async function\n");
        void (*async_reconstruct)(int,int,int) = dlsym(handle,"async_reconstruct");
        if (!async_reconstruct) {
            fprintf(stderr, "Error loading async_reconstruct function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        async_reconstruct(shm_id,shm_id,argn);

    }else{
        //printf("sync function\n");
        void (*sync_reconstruct)(int,int,int,int) = dlsym(handle,"sync_reconstruct");
        if (!sync_reconstruct) {
            fprintf(stderr, "Error loading sync_reconstruct function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        sync_reconstruct(buffer_size,argn,shm_id,shm_id_2);
    }
    pthread_mutex_lock(&args->mutex);
    args->reconstruct_done = 1;
    pthread_cond_signal(&args->cond_reconstruct);
    pthread_mutex_unlock(&args->mutex);
    dlclose(handle);
    return NULL;
}

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
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    }RingBuffer;

    int shm_id = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }
    int shm_id_2 = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
    if (shm_id_2 == -1) {
        perror("shmget");
        exit(1);
    }
    Args args;
    args.buffer_size = buffer_size;
    args.opt_arg = optional;
    args.shm_id = shm_id;
    args.shm_id2 = shm_id_2;
    // pthread_mutex_init(&args.mutex,NULL);
    // pthread_cond_init(&args.cond_observe,NULL);
    // pthread_cond_init(&args.cond_reconstruct,NULL);
    args.observe_done = 0;
    args.reconstruct_done = 0;
    if (is_sync == 1){
        strcpy(args.buffer_option,"sync");
    }else{
        strcpy(args.buffer_option,"async");
    }

    pthread_t observe_thread, reconstruct_thread, tapplot_thread;

    pthread_create(&observe_thread,NULL,run_observe,&args);
    //pthread_create(&reconstruct_thread,NULL,run_reconstruct,&args);
    //pthread_create(&tapplot_thread,NULL,run_tapplot,&args);
    pthread_join(observe_thread,NULL);
    //pthread_join(reconstruct_thread,NULL);
    //pthread_join(tapplot_thread,NULL);
    
    // pthread_mutex_destroy(&args.mutex);
    // pthread_cond_destroy(&args.cond_observe);
    // pthread_cond_destroy(&args.cond_reconstruct);

    // Delete shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
    // Delete shared memory segment
    if (shmctl(shm_id_2, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    return 0;
}