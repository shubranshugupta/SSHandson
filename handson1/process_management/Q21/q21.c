#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    // Call fork() to create a new process
    pid_t pid = fork();

    if (pid < 0) {
        // fork() returns a negative value if it fails
        perror("Fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // This block executes only in the CHILD process
        printf("--- CHILD PROCESS ---\n");
        printf("My own PID (Child): \t%d\n", getpid());
        printf("My Parent's PID: \t%d\n\n", getppid());
    } 
    else {
        // This block executes only in the PARENT process
        // wait(NULL) forces the parent to pause until the child finishes executing
        wait(NULL);
        
        printf("--- PARENT PROCESS ---\n");
        printf("My own PID (Parent): \t%d\n", getpid());
        printf("My Child's PID: \t%d\n", pid);
    }

    return 0;
}