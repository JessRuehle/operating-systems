/**
 * Custom shell.
 *
 * **KNOWN BUGS**
 * 
 * 1. Doesn't always handle single command input.
 * 
 * 2. Piping does not work
 * 
 * 3. Redirection is touchy, works sometimes.
 *
 * @author Jessica Ruehle
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CMD 100
#define MAX_ARGS 200
#define ARG_AT 300

struct Cmd {
    char args[MAX_ARGS][ARG_AT];
    char file_in[100];
    char file_out[100];
    int has_pipe;
    int num_args;
};

void execArgs(struct Cmd *, char **);

void printDir();

void tokenize(char *, struct Cmd *);


int main () {

    printf("Welcome to my shell! Enjoy your stay!\n");
    char input[100]; // command line input
    struct Cmd *newCmd; // stores info about input 
    char *argv[MAX_ARGS + 1];
    char *argv2[MAX_ARGS + 1];

    // print working directory for user
    printDir();

    // enter infinite loop that terminates when input is 'exit'
    while (1) {

        // get input from user and replace ending newline char with a null
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        // if the input is 'exit', loop breaks
        if (strcmp(input, "exit") == 0) {

            printf("Thank you for staying at my shell!\n");
            return 0;
        }

        // allocate memory for newCmd stuct and call tokenize to initlaize input
        newCmd = (struct Cmd *)malloc(sizeof(struct Cmd));
        tokenize(input, newCmd);

        // change directory built-in function
         if (strcmp(newCmd->args[0], "cd") == 0 && newCmd->args[1] != NULL) {

            chdir(newCmd->args[1]);
            printDir();

         } else {

            // fork process
            pid_t pid = fork();

            // if this is the child process,
            if (pid == 0) {

                // if there is a pipe, go here
                if (newCmd->has_pipe > 0) {

                    // gather commands from before the pipe
                    int i;
                    for (i = 0; i <= newCmd->has_pipe && newCmd->args[i][0] != '\0'; i++)
                        argv[i] = newCmd->args[i];
                    argv[i] = NULL;
                    
                    // gather commands from after the pipe
                    for (i = 0; i < MAX_ARGS && newCmd->args[i + newCmd->has_pipe][0] != '\0'; i++)
                        argv2[i] = newCmd->args[i + newCmd->has_pipe];
                    argv2[i] = NULL;

                    // handle pipe
                    int pipefd[2];
                    if (pipe(pipefd) == -1) {

                        // display error if pipe creation is unsuccessful
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }

                    // fork child process again
                    pid_t subpid = fork();

                    // if in the sub-child process
                    if (subpid == 0) { 

                        // close read end of pipe, redirect stdout to write, and close write end
                        close(pipefd[0]);
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);

                        // execute the command before the pipe
                        execArgs(newCmd, argv);

                    // if parent process,
                    } else if (subpid > 0) {

                        // close write end of pipe, redirect stdin to read end and close read end
                        close(pipefd[1]);
                        dup2(pipefd[0], STDIN_FILENO);
                        close(pipefd[0]);

                        // execute the command after the pipe
                        execArgs(newCmd, argv2);

                    // anything else would be a failure of the fork creation
                    } else {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }

                // if not piped, go here
                } else {
                    int i;
                    for (i = 0; i < newCmd->num_args && newCmd->args[i][0] != '\0'; i++)
                        argv[i] = newCmd->args[i];
                    argv[i] = NULL;

                    execArgs(newCmd, argv);
                }

            // parent process prints messge and waits for chlidren
            } else {
                wait(0);
            }
        }

        // free memory for newCmd struct
        free(newCmd);
    }

}

/*
Prints the current working directory.
*/
void printDir() {
    char pwd[100];
    getcwd(pwd, sizeof(pwd));
    printf("pwd: %s\n", pwd);
}

/*
Tokenizes user input.
*/
void tokenize(char *input, struct Cmd *command) {

    // Tokenize input based on spaces
    char *token = strtok(input, " ");
    int counter = 0;
    command->file_in[0] = '\0';
    command->file_out[0] = '\0';

    // Store each word in the words array
    while (token != NULL) {

        // if there is a redirection detected, store the filename    
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            strcpy(command->file_in, token);
        
        // if there is a redirection detected, store the filename
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            strcpy(command->file_out, token);

        // if there is a pipe detected, note its location
        } else if (strcmp(token, "|") == 0) {
            command->has_pipe = counter;
            token = strtok(NULL, " ");

        // any other case, add argument to newCmd
        } else {
        strcpy(command->args[counter], token);
        token = strtok(NULL, " ");
        }

        // increment counter
        counter++;
    }

    command->num_args = counter;
    command->args[counter + 1][0] = '\0';
}

void execArgs(struct Cmd *newCmd, char **argv) {

    // handle file redirection
    if (newCmd->file_in[0] != '\0') {
        int fd_in = open(newCmd->file_in, O_RDONLY);
        if (fd_in < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    // handle file redirection
    if (newCmd->file_out[0] != '\0') {
        int fd_out = open(newCmd->file_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // execute process
    if (execvp(argv[0], argv) < 0) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}