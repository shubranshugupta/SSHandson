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
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        printf("Child Process (PID: %d) is exiting immediately to become a Zombie.\n", getpid());
        
        // The child exits, but the parent isn't waiting for it!
        exit(0); 
    } 
    else {
        // --- PARENT PROCESS ---
        printf("Parent Process (PID: %d) created Child (PID: %d).\n", getpid(), pid);
        printf("Parent is going to sleep for 30 seconds WITHOUT calling wait().\n\n");
        
        printf("--> QUICK! Open another terminal and run:\n");
        printf("--> ps -l -p %d\n\n", pid);
        
        // Sleep keeps the parent running without checking on the child
        sleep(30);
        
        printf("Parent woke up and is exiting. (The zombie will now be cleared by the system).\n");
    }

    return 0;
}