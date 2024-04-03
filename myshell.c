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
void execute_pipeline(char *args[],int pipe_count,int background);
pid_t foreground_group_pid = 0; //Global variable to keep track of foreground processes

int main(){
    //Initialize a cmd char array for the command line from STDIN
    char cmd[MAX_SIZE_CMD];
    //Get the number of arguments from the terminal via array of strings
    char *args[MAX_SIZE_ARG];
    //Initialize process id to be used for later
    pid_t pid;
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
            fflush(stdout);
        }

        if (fgets(cmd,MAX_SIZE_CMD,stdin) == NULL){
            if (feof(stdin)){
                exit(EXIT_SUCCESS);
            }
            break;
        }
        //End the cmd char array with NULL terminated pointer at the end
        cmd[strlen(cmd) - 1] = '\0';
        //Initialize background and pipe count to 0
        background = 0;
        pipe_count = 0;
        parse_cmd(cmd,args,&background,&pipe_count);//A function to start parsing our command line
        
        //Exit out of the shell program prompted by the exit argument
        if (strcmp(args[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
            //break;
        }

        if (pipe_count > 0){//Check for pipe count to execute pipeline commands
            execute_pipeline(args,pipe_count,background);
        }else{//execute single normal and redirection commands 
            execute_command(args,background);
        }
        
    }
    return 0;
}
void execute_pipeline(char *args[], int pipe_count, int background) {
    int pipe_fd[2];//Initialize a pipe_fd array to return two file descriptors. 
    pid_t pid, pipeline_pid;//create another process id
    int arg_index = 0;
    char *current_args[MAX_SIZE_ARG];

    pipeline_pid = fork();
    if (pipeline_pid == 0) {
        // Child process for the entire pipeline
        while (args[arg_index] != NULL) {
            int i = 0;
            //Parse the command for each pipe
            while (args[arg_index] != NULL && strcmp(args[arg_index], "|") != 0) {
                current_args[i++] = args[arg_index++];
            }
            current_args[i] = NULL;

            if (args[arg_index] == NULL) {
                // Last command in the pipeline
                execute_command(current_args, background);
            } else {
                // Create a pipe and fork a child process
                pipe(pipe_fd);
                pid = fork();
                if (pid == 0) {
                    // Child process for the current command
                    //pipe_fd[1] (the write end of the pipe) is duplicated to the standard output (STDOUT_FILENO),
                    //so that any output from the command will be written to the pipe. 
                    close(pipe_fd[0]);
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[1]);
                    execute_command(current_args, background);//execute the command 
                    exit(0);
                } else if (pid < 0) {
                    fprintf(stderr, "Error forking");
                    exit(EXIT_FAILURE);
                } else {
                    //For the parent process, command after the pipe,
                    //pipe_fd[0] (the read end of the pipe) is duplicated to the standard input (STDIN_FILENO), 
                    //so that the command can read its input from the pipe. 
                    close(pipe_fd[1]);
                    dup2(pipe_fd[0], STDIN_FILENO);
                    close(pipe_fd[0]);
                    arg_index++;
                }
            }
        }
        exit(0);
    } else if (pipeline_pid < 0) {
        fprintf(stderr, "Error forking");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (!background) {
            foreground_group_pid = pipeline_pid;  // Store the foreground process group ID
            waitpid(pipeline_pid, NULL, 0);
            foreground_group_pid = 0;  // Reset the foreground process group ID
        }
    }
}
//Primarily the main function to execute our commands
void execute_command(char *args[], int background){
    int fd, pipe_count = 0; //Initialize variables for file descriptor and pipe count processes
    int pipe_fd[2]; //Initialize a pipe_fd array to return two file descriptors. 
    //pipe_fd[0] refers to the read end of the pipe. pipe_fd[1] refers to the write end of the pipe.
    pid_t pid;//create another process id
    pid = fork();
    if (pid == 0){//Checking if it's a child process
        //Redirect for input/output mainly the same process for all the redirection
        for (int i = 0; args[i] != NULL; i++){
            if (strcmp(args[i] , "<") == 0){
                //Open the file given in args[i+1] to read the file
                fd = open(args[i+1],O_RDONLY);
                dup2(fd,STDIN_FILENO);//fd is duplicated into the standard input (STDIN_FILENO)
                close(fd);//Close the fd after it's been read
                args[i] = args[i+1] = NULL; //Mark the positon of the argument array to NULL to indicate the cmd has been completed
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
        if (execvp(args[0],args) < 0){ //execute the command
            //fprintf(stderr, "myshell error: %s\n", strerror(errno) );
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0){
        fprintf(stderr,"Error forking");
        exit(EXIT_FAILURE);
    }else{
        //Parent process
        if (!background){
            // Wait for the child process to complete
            waitpid(pid, NULL,0);
        }else{
            printf("Background process started with PID %d\n",pid);
        }
    }
}
void handle_sigint(int sig){
    if (foreground_group_pid != 0){
        killpg(foreground_group_pid,SIGINT);
    }else{
        printf("\n");
        printf("myshell> ");
        fflush(stdout);//Flush the buffer back to standard out
        
    }
}

void handle_sigchild(int sig){
    //Wait for all child processes to terminate
    int status;
    pid_t child_pid;
    //will check if any zombie-children exist. If yes, one of them is reaped and its exit status returned. 
    //If not, either 0 is returned (if unterminated children exist) or -1 is returned (if not) 
    //printf("sigchld_handler called\n"); // Print statement to identify the function
    //-1 is to wait for any child processes
    //&status is a pointer to an integer where the exit status of the child process will be stored
    //WNOHANG flag tells waitpid to return immediately if no child process has exited; otherwise, it will block until a child process exits.
    while((child_pid = waitpid(-1,&status,WNOHANG)) > 0){
        // if (WIFEXITED(status)) {
        //     printf("Background process with PID %d finished with exit status %d\n", child_pid, WEXITSTATUS(status));
    
        // } else if (WIFSIGNALED(status)) {
        //     printf("Background process with PID %d terminated by signal %d\n", child_pid, WTERMSIG(status));
        // }
        // printf("\nmyshell> ");
        // fflush(stdout);
    }


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