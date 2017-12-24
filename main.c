#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define true            1
#define false           0
#define boolean unsigned char
#define MAX_INP_SIZE    1024
#define EXIT_COMMAND    "exit"
#define HELP_STRING     "A linux shell by Arad and Or Nevo.\nType 'exit' to exit.\n"

/* Headers */
// Receives the next input
void getInputString(char *dest);

// Returns an array of all passed parameters
char ** tokenize(char *toParse);

void launch(char **args, boolean isAmperPresent);

char* findRedirectionFile(char redirectionSym, char **args);

void redirect(char* redirectionFile, int redirectionFileno);

void executeCommand(char **args);

char ** findPipeCommand(char **args);

// Returns whether there's an '&' appended to the command.
// Removes it from the args.
boolean findAmper(char **args);


/* Main function */
int main(void) {
    char input[MAX_INP_SIZE], **args;

    // Print help
    puts(HELP_STRING);

    // Shell loop
    while(true) {
        // Get next program to run
        printf(">>> ");
        getInputString(input);
        args = tokenize(input);

        if(args[0] == NULL) // if no input
            continue; // retry

        // Act according to the input
        launch(args, findAmper(args));

        // Free allocated space
        free(args);
    }
}


/* Helpers */
// Handles the actual launching
void launch(char **args, boolean concurrently) {
    int exitStatus, childPID, redirectionFD, pipePD[2];
    char* redirectionFile = NULL;
    char **pipedCommand = NULL;

    // Test if should exit
    if(!strcmp(args[0], EXIT_COMMAND))
        exit(0);

    // Fork to make sure the called program does not replace the shell
    if((childPID = fork()) == 0) { // If in child process
        // Do file redirection
        if((redirectionFile = findRedirectionFile('>', args))) // If redirect output
            redirect(redirectionFile, STDOUT_FILENO);
        else if((redirectionFile = findRedirectionFile('<', args))) // If redirect input
            redirect(redirectionFile, STDIN_FILENO);
        else if((pipedCommand = findPipeCommand(args))) { // Pipe
            // Try to open pipe
            if(pipe(pipePD) == -1){
                puts("Failed to open pipe. Aborting...");
                exit(1);
            }

            // Execute commands
            if(fork() == 0) { // If in child process
                // Set the input of the child to the corresponding side of the pipe
                dup2(pipePD[0], STDIN_FILENO);
                // execute the pipe-right-side command
                executeCommand(pipedCommand);
            } else
                // If in parent process, set output to the corresponding side of the pipe
                // We execute the command in a few lines, so no need to re-execute here
                dup2(pipePD[1], STDOUT_FILENO);

        }

        executeCommand(args);
    }

    // If parent process and no &, wait for child
    if(!concurrently)
        waitpid(childPID, &exitStatus, 0);
    else
        printf("[%d]\n", childPID);
}

/* General helpers */
// This handles the redirection of stream with fd 'redirectionFileno'
//  to the file named 'redirectionFile'
void redirect(char* redirectionFile, int redirectionFileno){
    int redirectionFD = 0, access;

    // Determine the FD to replace
    if(redirectionFileno == STDOUT_FILENO) access = O_WRONLY;
    else access = O_RDONLY;

    // Try to open the redirectionFile
    redirectionFD = open(redirectionFile, (access | O_CREAT), 0777);//(S_IWRITE | S_IREAD));

    // If failed to open
    if(redirectionFD == -1) {
        printf("Failed to open file %s for redirection.\n", redirectionFile);
        exit(1);
    }

    // Try to replace the FD
    if(dup2(redirectionFD, redirectionFileno) == -1) {
        puts("Failed to redirect io. Aborting...");
        exit(1);
    }

    close(redirectionFD);
}

void executeCommand(char **args) {
    if(execvp(args[0], args) == -1) { // If error
        printf("Failed to launch \"%s\".\n", args[0]);
        exit(1);
    }
}

/* Parameters parsing */
// Returns whether there's an '&' appended to the command.
// Removes it from the args.
boolean findAmper(char **args) {
    int i;
    for(i = 0; args[i]; i++) {
        // If last parameter and is &
        if(args[i+1] == NULL && !strcmp(args[i], "&")) {
            // Terminate the args earlier
            args[i] = NULL;
            return true;
        }
    }

    return false;
}

// This function tests for io redirection, and
//  if exists returns the file name.
//  else returns NULL
// :param redirectionSym: either '>' or '<'. What to look for.
char* findRedirectionFile(char redirectionSym, char **args) {
    int i;
    for (i = 0; args[i]; i++) {
        if(args[i][0] == redirectionSym) {
            // If there is more than one parameter after the '>/<'
            // Or the parameter where the file name should be is empty
            if(args[i+2] || !args[i+1]){
                puts("Illegal file name for io redirection. Aborting...");
                exit(1);
            }
            // If legal
            args[i] = NULL; // Terminate parameters to execvp before redirection
            return args[i+1];
        }
    }

    return NULL;
}

// This function find a pipe in the command, and returns the command
//  which the output should be piped into
// Returns NULL if no pipe was specified.
char ** findPipeCommand(char **args){
    int i;
    for (i = 0; args[i]; i++)
        if(args[i][0] == '|' && args[i+1]) {
            args[i] = NULL;
            return &(args[i+1]);
        }

    return NULL;
}

// This function receives the next input
void getInputString(char *dest) {
    int i = 0;
    char curr;

    // get up to MAX_INP_SIZE characters
    for (i = 0; i < MAX_INP_SIZE - 1; ++i) {
        curr = getchar();
        if(curr == '\n') break;

        *dest = curr;
        dest++;
    }

    // NULL terminate
    *dest = '\0';
}


// Returns an array of all passed parameters
char ** tokenize(char *toParse) {
    int inputLength = strlen(toParse), i = 0,
        maxArgsNum = inputLength/2 + inputLength%2; // Assuming every param is 1 character long
    char *tmp, **args = (char **)calloc(maxArgsNum + 1, sizeof(char *)); // The +1 is for the NULL terminetor

    // Get the tokens
    tmp = strtok(toParse, " ");

    while(tmp != NULL) {
        args[i++] = tmp;
        tmp = strtok(NULL, " ");
    }

    // Null terminate the array
    args[i++] = NULL;

    return args;
}
