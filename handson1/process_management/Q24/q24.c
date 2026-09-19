#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    // Create a new process
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } 
    else if (pid > 0) {
        // --- PARENT PROCESS ---
        printf("Parent Process (PID: %d) created Child (PID: %d).\n", getpid(), pid);
        printf("Parent is exiting immediately! (Leaving the child behind...)\n");
        
        // Parent dies instantly
        exit(0); 
    } 
    else {
        // --- CHILD PROCESS ---
        printf("\n--- CHILD PROCESS ---\n");
        printf("My own PID: %d\n", getpid());
        printf("My Parent's PID before orphan: %d\n", getppid());
        
        // Sleep for 5 seconds to guarantee the parent dies first
        printf("Child is sleeping for 5 seconds...\n\n");
        sleep(5);
        
        // By the time the child wakes up, the parent is gone.
        printf("--- CHILD WOKE UP ---\n");
        printf("My Parent's PID after orphan: %d\n", getppid());
        printf("My PID after orphan: %d\n", getpid());
        printf("Notice the Parent PID changed! I have been adopted.\n");
    }

    return 0;
}