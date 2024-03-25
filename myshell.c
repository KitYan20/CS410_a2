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

void parse_cmd(char *cmd, char *args[], int *background, int *pipe_count);
void handle_sigint(int sig);
void handle_sigchild(int sig);
void execute_command(char *args[], int background);

int main(){
    //signal(SGNCHLD, log_handle);
    char cmd[MAX_SIZE_CMD];
    char *args[MAX_SIZE_ARG];
    pid_t pid, child_pid;

    int background,pipe_count,status;

    signal(SIGINT,handle_sigint);
    signal(SIGCHLD,handle_sigchild);

    while(1){
        if (isatty(STDIN_FILENO)){
            printf("myshell> ");
        }

        if (fgets(cmd,MAX_SIZE_CMD,stdin) == NULL){
            break;
        }

        cmd[strlen(cmd) - 1] = '\0';
        background = pipe_count = 0;
        parse_cmd(cmd,args,&background,&pipe_count);

        char *current_args[MAX_SIZE_ARG];
        int current_background, current_pipe_count;
        int arg_index = 0;

        while (args[arg_index] != NULL) {
            current_background = background;
            current_pipe_count = pipe_count;
            int i = 0;
            while (args[arg_index] != NULL) {
                current_args[i++] = args[arg_index++];
            }
            current_args[i] = NULL;
            arg_index++;

            pid = fork();

            if (pid == 0) {
                // Child process
                execute_command(current_args, current_background);
                exit(0);
            } else if (pid < 0) {
                // Error forking
                fprintf(stderr, "Error forking");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                if (!current_background) {
                    waitpid(pid, &status, 0);
                } else {
                    printf("Child PID %d\n", pid);
                }
            }
        }

        if (strcmp(args[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
            //break;
        }

        
    }
    return 0;
}
void execute_command(char *args[], int background){
    int fd, pipe_count = 0;
    int pipe_fd[2];
    pid_t pid;

    for (int i = 0; args[i] != NULL; i++){
        if (strcmp(args[i], "|") == 0){
            pipe(pipe_fd);
            pid = fork();
            if (pid == 0){
                //child process
                dup2(pipe_fd[1],STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                execute_command(args,background);
                exit(0);
            }else if (pid  < 0){
                //Error forking
                fprintf(stderr,"Error forking");
                exit(EXIT_FAILURE);
            }else{
                //parent process
                dup2(pipe_fd[0],STDIN_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                args[i] = NULL;
                pipe_count++;
            }
        }
    }
    //Redirect for input/output
    if (pipe_count == 0){
        for (int i = 0; args[i] != NULL; i++){
            if (strcmp(args[i] , "<") == 0){
                fd = open(args[i+1],O_RDONLY);
                dup2(fd,STDIN_FILENO);
                close(fd);
                args[i] = NULL;
                args[i+1] = NULL;
            } else if (strcmp(args[i] , ">") == 0){
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "1>") == 0){
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "2>") == 0){
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDERR_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "&>") == 0){
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            }
        }
    }

    if (!background){
        if (execvp(args[0],args) < 0){
            //fprintf(stderr, "myshell error: %s\n", strerror(errno) );
            exit(EXIT_FAILURE);
        }
       
    }else{
        pid = fork();

        if (pid == 0){
            if (execvp(args[0],args) < 0){
                //fprintf(stderr, "myshell error: %s\n", strerror(errno) );
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0){
            fprintf(stderr,"Error forking");
            exit(EXIT_FAILURE);
        }
    }
}
void handle_sigint(int sig){
    // Send SIGINT to all child processes
    
    pid_t foreground_group_pid = tcgetpgrp(STDIN_FILENO);
    if (foreground_group_pid != -1){
        killpg(foreground_group_pid,SIGINT);
    }

}

void handle_sigchild(int sig){
    //Wait for all child processes to terminate
    int status;
    //will check if any zombie-children exist. If yes, one of them is reaped and its exit status returned. 
    //If not, either 0 is returned (if unterminated children exist) or -1 is returned (if not) 
    while(waitpid(-1,&status,WNOHANG) > 0);

}

//Conver the command line from stdin to a array of arguments to parse through
void parse_cmd(char *cmd, char *args[], int *background, int *pipe_count){
    int arg_count = 0;
    char *token, *rest = cmd;
    char *saveptr; //for strtok_r
    token = strtok_r(rest, ";", &saveptr);
    
    while(token != NULL){
        char *cmd_args[MAX_SIZE_ARG];
        int cmd_background = 0;
        int cmd_pipe_count = 0;
        int cmd_arg_count = 0;

        char *cmd_token, *cmd_rest = token;

        while((cmd_token = strtok_r(cmd_rest," ",&cmd_rest))){
            //Background task running
            if (strcmp(cmd_token, "&") == 0){
                cmd_background = 1;
            } else if (strcmp(cmd_token,"|") == 0){
                cmd_pipe_count++;
                cmd_args[cmd_arg_count++] = cmd_token;
            }else{
                cmd_args[cmd_arg_count++] = cmd_token;
            }
        }
        cmd_args[cmd_arg_count] = NULL;

        for (int i = 0; i < cmd_arg_count; i++) {
            args[arg_count++] = cmd_args[i];
        }
        args[arg_count++] = NULL;

        *background = cmd_background;
        *pipe_count += cmd_pipe_count;

        token = strtok_r(NULL, ";", &saveptr);
    }
    
}