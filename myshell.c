#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAX_SIZE_CMD 256
#define MAX_SIZE_ARG 16

char cmd[MAX_SIZE_CMD];
char *argv[MAX_SIZE_ARG];
pid_t pid;
char i;

void get_cmd();
void convert_cmd();
void shell();
//void log_handle(int sig);

int main(){
    //signal(SGNCHLD, log_handle);
    shell();
    return 0;
}

void shell(){
    while(1){
        //get command from a user
        get_cmd();

        if (!strcmp("",cmd)){
            continue;
        }
        if (!strcmp("exit",cmd)){
            break;
        }
        convert_cmd();
        //fork and execute the command
        pid = fork();
        
        if (-1 == pid){
            printf("failed to create a child\n");
        }else if (0 == pid){
            //printf("hello from child\n");

            execvp(argv[0],argv);
        }else{
            // printf("hello from parent\n");
			// wait for the command to finish if "&" is not present
			if(NULL == argv[i]){
                waitpid(pid, NULL, 0);
    
            } 
        }

    }
}

void get_cmd(){
    printf("myshell> ");
    fgets(cmd,MAX_SIZE_CMD,stdin); //Get a command from a user
    if ((strlen(cmd) > 0) && (cmd[strlen(cmd) -  1] == '\n')){
        cmd[strlen(cmd) - 1] = '\0';
    }
}

void convert_cmd(){
    //Split the string into argv
    char *ptr;
    i = 0;
    ptr = strtok(cmd," ");
    while(ptr != NULL){
        argv[i] = ptr;
        i++;
        ptr = strtok(NULL," ");
    }
    //Checks for & 
    if (!strcmp("&", argv[i-1])){
        argv[i-1] = NULL;
        argv[i] = "&";
    }else{
        argv[i] = NULL;
    }
}