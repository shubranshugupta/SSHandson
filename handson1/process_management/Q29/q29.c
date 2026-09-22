#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>

// Helper function to decode and print the current scheduling policy
void print_current_policy(pid_t pid) {
    int policy = sched_getscheduler(pid);
    
    if (policy == -1) {
        perror("Error getting scheduler policy");
        return;
    }

    printf("Current Scheduling Policy: ");
    switch(policy) {
        case SCHED_OTHER: 
            printf("SCHED_OTHER (Standard Time-Sharing)\n"); 
            break;
        case SCHED_FIFO:  
            printf("SCHED_FIFO (Real-Time First-In, First-Out)\n"); 
            break;
        case SCHED_RR:    
            printf("SCHED_RR (Real-Time Round Robin)\n"); 
            break;
        default:          
            printf("UNKNOWN\n");
    }
}

int main() {
    pid_t pid = getpid();
    struct sched_param param;

    printf("Program started with PID: %d\n", pid);
    
    // 1. Get and print the default policy
    print_current_policy(pid);

    // ==========================================
    // 2. Switch to SCHED_FIFO
    // ==========================================
    printf("\nAttempting to switch to SCHED_FIFO...\n");
    
    // We must set the real-time priority in the sched_param structure.
    // We will use the maximum priority allowed for FIFO.
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    
    // sched_setscheduler(PID, Policy, &param)
    if (sched_setscheduler(pid, SCHED_FIFO, &param) == -1) {
        perror("FAILED");
        printf("Hint: Did you forget to run the program with 'sudo'?\n");
        return 1;
    }
    
    printf("SUCCESS! Priority set to: %d\n", param.sched_priority);
    print_current_policy(pid);


    // ==========================================
    // 3. Switch to SCHED_RR
    // ==========================================
    printf("\nAttempting to switch to SCHED_RR...\n");
    
    // Set the real-time priority for Round Robin
    param.sched_priority = sched_get_priority_min(SCHED_RR);
    
    if (sched_setscheduler(pid, SCHED_RR, &param) == -1) {
        perror("FAILED");
        return 1;
    }
    
    printf("SUCCESS! Priority set to: %d\n", param.sched_priority);
    print_current_policy(pid);

    return 0;
}