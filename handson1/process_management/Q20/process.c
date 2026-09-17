#include <stdio.h>
#include <unistd.h>

int main() {
    // Call getpid() to retrieve the current process ID
    pid_t pid = getpid();

    // Print the process ID to the console
    printf("Process ID (PID): %d\n", pid);

    while (1) {
        // Infinite loop to keep the process running
        sleep(5); // Sleep for 5 seconds to reduce CPU usage
    }

    return 0;
}