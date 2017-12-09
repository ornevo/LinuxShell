#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    int exitStatus, childPID;

    // Test if should exit
    if(!strcmp(args[0], EXIT_COMMAND))
        exit(0);

    // Fork to make sure the called program does not replace the shell
    if((childPID = fork()) == 0) { // If in child process
        if(execvp(args[0], args) == -1) { // If error
            printf("Failed to launch \"%s\".\n", args[0]);
            exit(1);
        }
    }

    // If parent process and no &, wait for child
    if(!concurrently)
        waitpid(childPID, &exitStatus, 0);
    else
        printf("[%d]\n", childPID);
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