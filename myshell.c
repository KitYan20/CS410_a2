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

//Define all of our functions in the file 
void parse_cmd(char *cmd, char *args[], int *background, int *pipe_count);
void handle_sigint(int sig);
void handle_sigchild(int sig);
void execute_command(char *args[], int background);

int main(){
    //Initialize a cmd char array for the command line from STDIN
    char cmd[MAX_SIZE_CMD];
    //Get the number of arguments from the terminal via array of strings
    char *args[MAX_SIZE_ARG];
    //Initialize process id to be used for later
    pid_t pid, child_pid;
    //Initialize a background to check whether to have a command execute on the background
    //A pipe count to check how many pipeline processes if there are in the terminal
    //A status to check the status of the pid
    int background,pipe_count,status;

    signal(SIGINT,handle_sigint);
    signal(SIGCHLD,handle_sigchild);

    //Initialize our shell in a while loop to continously take commands from user until exit
    while(1){
        //checks if the file descriptor is referring to a terminal 
        if (isatty(STDIN_FILENO)){
            //initialize the prompt to get ready for user input with "myshell" prompt
            printf("myshell> ");
        }

        if (fgets(cmd,MAX_SIZE_CMD,stdin) == NULL){
            break;
        }
        //End the cmd char array with NULL terminated pointer at the end
        cmd[strlen(cmd) - 1] = '\0';
        //Initialize background and pipe count to 0
        background = pipe_count = 0;
        parse_cmd(cmd,args,&background,&pipe_count);//A function to start parsing our command line
        char *current_args[MAX_SIZE_ARG];//Create another argument string to MAX_SIZE_ARG
        int current_background, current_pipe_count;
        int arg_index = 0;
        //Another while loop to fill our current_args array and updating background and pipe_count
        while (args[arg_index] != NULL) {
            current_background = background;
            current_pipe_count = pipe_count;
            int i = 0;
            //Just filling in the argument array 
            while (args[arg_index] != NULL) {
                current_args[i++] = args[arg_index++];
            }
            current_args[i] = NULL;
            arg_index++;
            pid = fork();//Create a child process
            if (pid == 0) {
                // Child process and execute the current command arguments with the current background status
                execute_command(current_args, current_background);
                exit(0);
            } else if (pid < 0) {
                // Error forking
                fprintf(stderr, "Error forking");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                if (!current_background) {
                    //Parent can collect the exit status of the child to prevent creation of a Zombie  process
                    waitpid(pid, &status, 0);// allows the parent process to wait for a specific child process to terminate. 
                } else {
                    printf("I am parent %d status %d\n", pid,status);
                }
            }
        }
        //Exit out of the shell program prompted by the exit argument
        if (strcmp(args[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
            //break;
        }
        
    }
    return 0;
}
//Primarily the main function to execute our commands
void execute_command(char *args[], int background){
    int fd, pipe_count = 0; //Initialize variables for file descriptor and pipe count processes
    int pipe_fd[2]; //Initialize a pipe_fd array to return two file descriptors. 
    //pipe_fd[0] refers to the read end of the pipe. pipe_fd[1] refers to the write end of the pipe.
    pid_t pid;//create another process id
    for (int i = 0; args[i] != NULL; i++){ // A for loop to check for "|" processes
        if (strcmp(args[i], "|") == 0){
            pipe(pipe_fd); //Use the pipe function to create a new pipe
            pid = fork(); //create a new process
            if (pid == 0){
                //child process
                //pipe_fd[1] (the write end of the pipe) is duplicated to the standard output (STDOUT_FILENO),
                //so that any output from the command will be written to the pipe. 
                //Then, the original file descriptors (pipe_fd[0] and pipe_fd[1]) are closed, as they are no longer needed in the child process.
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
                //For the parent process, command after the pipe,
                //pipe_fd[0] (the read end of the pipe) is duplicated to the standard input (STDIN_FILENO), 
                //so that the command can read its input from the pipe. 
                //Again, the original file descriptors (pipe_fd[0] and pipe_fd[1]) are closed, as they are no longer needed in the parent process.
                dup2(pipe_fd[0],STDIN_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                args[i] = NULL;
                pipe_count++;
            }
        }
    }
    //Redirect for input/output mainly the same process for all the redirection
    if (pipe_count == 0){
        for (int i = 0; args[i] != NULL; i++){
            if (strcmp(args[i] , "<") == 0){
                //Open the file given in args[i+1] to read the file
                fd = open(args[i+1],O_RDONLY);
                dup2(fd,STDIN_FILENO);//fd is duplicated into the standard input (STDIN_FILENO)
                close(fd);//Close the fd after it's been read
                args[i] = args[i+1] = = NULL; //Mark the positon of the argument array to NULL to indicate the cmd has been completed
                //Along with the argument where the location of using the command
            } else if (strcmp(args[i] , ">") == 0){//redirect to stdout
                //Same process for the rest of the redirection but now we are writing to the file indicated by args[i+1]
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);//Duplicate the file descriptor
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "1>") == 0){//redirect to stdout indicated by 1
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "2>") == 0){//redirect to stderr indicated by 2
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDERR_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            } else if (strcmp(args[i] , "&>") == 0){
                fd = open(args[i+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
                //Duplicate the file descriptor to both standard out and standard error indicated by &>
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
                args[i] = args[i + 1] = NULL;
            }
        }
    }

    if (!background){//checks to see if no background process is intiated
        if (execvp(args[0],args) < 0){//use execvp to execute the command indicated by args[0] and whatever parameter from the args string
            //fprintf(stderr, "myshell error: %s\n", strerror(errno) );
            exit(EXIT_FAILURE);
        }
       
    }else{
        pid = fork(); //Creates a child process
        if (pid == 0){//Checking if it's a child process
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

//Convert the command line from stdin to a array of arguments to parse through
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