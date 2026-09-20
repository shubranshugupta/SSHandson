#include <stdio.h>
#include <sched.h>
#include <stdlib.h>

int main() {
    int min, max;

    printf("--- Real-Time Scheduling Policies ---\n");

    // 1. SCHED_FIFO: First In, First Out (Real-Time)
    // Processes run until they yield, block, or are preempted by a higher priority RT task.
    max = sched_get_priority_max(SCHED_FIFO);
    min = sched_get_priority_min(SCHED_FIFO);
    if (max == -1 || min == -1) perror("Error getting SCHED_FIFO limits");
    printf("SCHED_FIFO  -> Minimum: %d \t Maximum: %d\n", min, max);

    // 2. SCHED_RR: Round Robin (Real-Time)
    // Similar to FIFO, but processes at the same priority take turns using time slices.
    max = sched_get_priority_max(SCHED_RR);
    min = sched_get_priority_min(SCHED_RR);
    if (max == -1 || min == -1) perror("Error getting SCHED_RR limits");
    printf("SCHED_RR    -> Minimum: %d \t Maximum: %d\n\n", min, max);

    printf("--- Standard Time-Sharing Policy ---\n");

    // 3. SCHED_OTHER: Standard Linux scheduling
    // This is the default scheduler that uses "nice" values, not RT priorities.
    max = sched_get_priority_max(SCHED_OTHER);
    min = sched_get_priority_min(SCHED_OTHER);
    if (max == -1 || min == -1) perror("Error getting SCHED_OTHER limits");
    printf("SCHED_OTHER -> Minimum: %d \t Maximum: %d\n", min, max);

    return 0;
}