#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t child_pids[3];
    int status;

    printf("Parent Process (PID: %d) starting...\n\n", getpid());

    // 1. Create 3 child processes
    for (int i = 0; i < 3; i++) {
        child_pids[i] = fork();

        if (child_pids[i] < 0) {
            perror("Fork failed");
            return 1;
        } 
        else if (child_pids[i] == 0) {
            // --- CHILD PROCESS LOGIC ---
            // Each child sleeps for a different duration: 2s, 4s, and 6s
            int sleep_time = (i + 1) * 2;
            printf("  -> Child %d (PID: %d) started. Sleeping for %d seconds...\n", i + 1, getpid(), sleep_time);
            
            sleep(sleep_time);
            
            printf("  <- Child %d (PID: %d) finished and exiting.\n", i + 1, getpid());
            exit(0); // Child must exit so it doesn't continue the loop!
        }
    }

    // --- PARENT PROCESS LOGIC ---
    // The parent has successfully forked 3 times and stored their PIDs.
    
    printf("\n[PARENT] All 3 children created successfully.\n");
    printf("[PARENT] I am choosing to wait specifically for Child 2 (PID: %d)...\n\n", child_pids[1]);

    // 2. Call waitpid() for a specific child
    // First argument is the exact PID to wait for.
    // 0 as the third argument means "Block (freeze) until this specific process exits."
    pid_t completed_pid = waitpid(child_pids[1], &status, 0);

    if (completed_pid == child_pids[1]) {
        printf("\n[PARENT] Success! My target Child 2 (PID: %d) has finished.\n", completed_pid);
    }

    // 3. Clean up the remaining children
    // Even though we waited for Child 2, Child 3 is still running, and Child 1 is a zombie 
    // (since it finished first but nobody waited for it). 
    // We call a general wait() loop to clean up all remaining children before the parent exits.
    printf("[PARENT] Cleaning up the remaining children...\n");
    while (wait(NULL) > 0); 
    
    printf("[PARENT] All children cleaned up. Parent exiting.\n");

    return 0;
}