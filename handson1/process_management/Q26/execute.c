#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: Incorrect number of arguments.\n");
        return 1;
    }

    printf("Launcher: Starting up. Creating a child process...\n");
    
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        printf("Launcher (Child): I am replacing myself with the target executable.\n\n");
        
        // Prepare the arguments for the new program.
        // Rule 1: args[0] must conventionally be the name of the program.
        // Rule 2: The array MUST be terminated by a NULL pointer.
        // execve takes the path to the executable, and the array of arguments.
        execve(argv[1], &argv[1], NULL);
        
        // ==========================================
        // CRITICAL CONCEPT: 
        // If execve() is successful, it NEVER returns. 
        // The process image is completely overwritten. 
        // The code below will ONLY run if execve fails (e.g., file not found).
        // ==========================================
        perror("execve failed (Did you forget to compile?)");
        exit(1);
    } 
    else {
        // --- PARENT PROCESS ---
        // Wait for the child (which is now running './greet') to finish
        wait(NULL);
        
        printf("\nLauncher (Parent): The target executable has finished. Exiting.\n");
    }

    return 0;
}