#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        printf("Shell> ");
        
        /*Reads a line of input from the user from the standard input (stdin) and stores it in the variable command */
        fgets(command, sizeof(command), stdin);
        
        /* Removes the newline character (\n) from the end of the string stored in command, if present. 
           This is done by replacing the newline character with the null character ('\0').
           The strcspn() function returns the length of the initial segment of command that consists of 
           characters not in the string specified in the second argument ("\n" in this case). */
        command[strcspn(command, "\n")] = '\0';

        // Skip if no commands were entered
        if (command_count == 0) continue;

        // Reset command_count for new input
        command_count = 0;

        /* Tokenizes the command string using the pipe character (|) as a delimiter using the strtok() function. 
           Each resulting token is stored in the commands[] array. 
           The strtok() function breaks the command string into tokens (substrings) separated by the pipe character |. 
           In each iteration of the while loop, strtok() returns the next token found in command. 
           The tokens are stored in the commands[] array, and command_count is incremented to keep track of the number of tokens found. */
        char *token = strtok(command, "|");
        while (token != NULL) 
        {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        /* You should start programming from here... */
        // Skip if no commands were entered
        if (command_count == 0) continue;

        // Create pipes for each pair of consecutive commands
        int pipes[MAX_COMMANDS-1][2];
        for (int i = 0; i < command_count-1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        // Process each command
        for (int i = 0; i < command_count; i++) {
            pid_t pid = fork();
            
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } 
            else if (pid == 0) { // Child process
                // Set up input from previous command (if not first command)
                if (i > 0) {
                    dup2(pipes[i-1][0], STDIN_FILENO);
                }
                
                // Set up output to next command (if not last command)
                if (i < command_count-1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }
                
                // Close all pipe file descriptors
                for (int j = 0; j < command_count-1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                // Parse command into arguments
                char *args[64];
                int arg_count = 0;
                
                char *arg = strtok(commands[i], " \t");
                while (arg != NULL) {
                    args[arg_count++] = arg;
                    arg = strtok(NULL, " \t");
                }
                args[arg_count] = NULL;
                
                // Execute command
                execvp(args[0], args);
                
                // If execvp fails
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        
        // Parent process: close all pipe file descriptors
        for (int i = 0; i < command_count-1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        
        // Wait for all child processes to complete
        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }
        
        // Reset command_count for next input
        command_count = 0;
    }
    return 0;
}
