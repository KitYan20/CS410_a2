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
#define MAX_VALUE_SIZE 1064
#define MAX_THREADS 10

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
    //printf("%d %s %d\n",buffer_size,buffer_option,shm_id);
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
    dlclose(handle);
    return NULL;
}

void *run_tapplot(void *arg){
    Args *args = (Args*)arg;
    int buffer_size = args->buffer_size;
    int shm_id = args->shm_id;
    int shm_id_2 = args->shm_id2;
    int argn = args->opt_arg;
    char *buffer_option = args->buffer_option;

    void *handle;
    handle = dlopen("./sharedlibrary.so", RTLD_LAZY);

    if (!handle) {
        fprintf(stderr, "Error loading shared library %s\n", dlerror());
        exit(1);
    }
    if (strcmp(buffer_option,"async") == 0){
        void (*async_tapplot)(int, int) = dlsym(handle, "async_plot");
        if (!async_tapplot) {
            fprintf(stderr, "Error loading tapplot function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        async_tapplot(argn,shm_id_2);
    }
    else{
        void (*sync_tapplot)(int, int,int) = dlsym(handle, "sync_plot");
        if (!sync_tapplot) {
            fprintf(stderr, "Error loading tapplot function: %s\n", dlerror());
            dlclose(handle);
            exit(1);
        }
        sync_tapplot(buffer_size,argn,shm_id_2);
    }

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
    // printf("tappet");
    // for (int i = 0 ; i < num_programs ; i++){
    //     printf(" -p%d %s",i+1, programs[i].name);
    // }
    // printf(" -o %d -b %d -s %d\n", optional,is_sync,buffer_size);
    //Each thread corresponds to one program 
    typedef struct {
        char data[buffer_size][MAX_VALUE_SIZE];
        int in;
        int out;
        int done;
    }RingBuffer;

    int shm_id, shm_id_2;
    Four_Slot_Buffer *four_slot_buffer;
    //Four_Slot_Buffer *four_slot_buffer_2;

    Args args;
    args.buffer_size = buffer_size;
    args.opt_arg = optional;
    
    if (is_sync == 1){
        strcpy(args.buffer_option,"sync");
        shm_id = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
        if (shm_id == -1) {
            perror("shmget");
            exit(1);
        }
        shm_id_2 = shmget(IPC_PRIVATE,sizeof(RingBuffer), IPC_CREAT | 0666);//Shared Memory ID
        if (shm_id_2 == -1) {
            perror("shmget");
            exit(1);
        }
        args.shm_id = shm_id;
        args.shm_id2 = shm_id_2;
    }else{
        strcpy(args.buffer_option,"async");
        shm_id = shmget(IPC_PRIVATE,sizeof(Four_Slot_Buffer), IPC_CREAT | 0666);//Shared Memory ID
        if (shm_id == -1) {
            perror("shmget");
            exit(1);
        }
        four_slot_buffer = (Four_Slot_Buffer *)shmat(shm_id, NULL, 0);
        if (four_slot_buffer == (void *)-1) {
            perror("shmat");
            exit(1);
        }
        
        shm_id_2 = shmget(IPC_PRIVATE,sizeof(Four_Slot_Buffer), IPC_CREAT | 0666);//Shared Memory ID
        if (shm_id_2 == -1) {
            perror("shmget");
            exit(1);
        }
        // four_slot_buffer_2 = (Four_Slot_Buffer *)shmat(shm_id_2, NULL, 0);
        // if (four_slot_buffer_2 == (void *)-1) {
        //     perror("shmat");
        //     exit(1);
        // }
        sem_init(&four_slot_buffer->mutex, 1, 1);
        sem_init(&four_slot_buffer->empty_slots, 1, 4);
        sem_init(&four_slot_buffer->full_slots, 1, 0);
        four_slot_buffer->in = 0;
        four_slot_buffer->out = 0;
        four_slot_buffer->done = 0;

        // sem_init(&four_slot_buffer_2->mutex, 1, 1);
        // sem_init(&four_slot_buffer_2->empty_slots, 1, 4);
        // sem_init(&four_slot_buffer_2->full_slots, 1, 0);
        // four_slot_buffer_2->in = 0;
        // four_slot_buffer_2->out = 0;
        // four_slot_buffer_2->done = 0;
        args.shm_id = shm_id;
        args.shm_id2 = shm_id_2;
    }    

    pthread_t observe_thread, reconstruct_thread, tapplot_thread;

    pthread_create(&observe_thread,NULL,run_observe,&args);
    pthread_create(&reconstruct_thread,NULL,run_reconstruct,&args);
    
    pthread_join(observe_thread,NULL);
    pthread_join(reconstruct_thread,NULL);

    pthread_create(&tapplot_thread,NULL,run_tapplot,&args);
    pthread_join(tapplot_thread,NULL);

    sem_destroy(&four_slot_buffer->mutex);
    sem_destroy(&four_slot_buffer->empty_slots);
    sem_destroy(&four_slot_buffer->full_slots);

    // sem_destroy(&four_slot_buffer_2->mutex);
    // sem_destroy(&four_slot_buffer_2->empty_slots);
    // sem_destroy(&four_slot_buffer_2->full_slots);

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