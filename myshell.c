#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#define MAX_SIZE_CMD 256
#define MAX_SIZE_ARG 16

char cmd[MAX_SIZE_CMD];
char *argv[MAX_SIZE_ARG];
pid_t pid;
char i;

void get_cmd();
void convert_cmd();
void shell();

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
        if (-1 == pid){ //Can't create child process
            printf("failed to create a child\n");
        }else if (0 == pid){//Able to create child process
            //printf("hello from child\n");
            char *output_file = NULL;
            char *input_file = NULL;
            for (int i = 0; argv[i] != NULL; i++){
                if (strcmp(argv[i],">") == 0){
                    output_file = argv[i+1];
                    argv[i] = NULL; //Remove the ">" redirection
                    argv[i+1] = NULL; //Remove the file
                    break;
                }else if (strcmp(argv[i],"<") == 0){
                    input_file = argv[i+1];
                    argv[i] = NULL;
                    argv[i+1] = NULL;
                    break;
                }
            }
            if (output_file != NULL){
                int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out == -1){
                    fprintf( stderr, "mysh error: can't open %s\n", output_file);
                    exit(1);
                }
                dup2(fd_out,STDOUT_FILENO);
                close(fd_out);
            }
            if (input_file != NULL){
                int fd_in = open(input_file, O_RDONLY);
                if (fd_in == -1){
                    fprintf( stderr, "mysh error: can't open %s\n", input_file);
                    exit(1);
                }
                dup2(fd_in,STDIN_FILENO);
                close(fd_in);
            }
            execvp(argv[0],argv);

            if( execvp(cmd, argv) == -1 ){
                //fprintf(stderr, "myshell error: %s\n", strerror(errno) );
                exit(EXIT_FAILURE);
            }     
        }else{
            // printf("hello from parent\n");
			// wait for the command to finish if "&" is not present
            int status;
            waitpid(pid, NULL, 0);
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

//Conver the command line from stdin to a array of arguments to parse through
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