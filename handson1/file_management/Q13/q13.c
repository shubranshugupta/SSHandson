#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

int main() {
    fd_set readfds;
    struct timeval timeout;
    int retval;

    // 1. Initialize the file descriptor set
    FD_ZERO(&readfds);
    
    // 2. Add STDIN (File Descriptor 0) to the read set
    FD_SET(STDIN_FILENO, &readfds);

    // 3. Configure the timeout for exactly 10 seconds and 0 microseconds
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    printf("Waiting for input on STDIN for up to 10 seconds...\n");

    // 4. Call select()
    // nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
    // Since STDIN is 0, nfds is 0 + 1 = 1.
    retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    // 5. Check the return value of select()
    if (retval == -1) {
        // An error occurred during the system call
        perror("select()");
        return EXIT_FAILURE;
    } 
    else if (retval == 0) {
        // select() returned 0, meaning the timer expired before any file descriptors became ready
        printf("\n[TIMEOUT] No data was entered on STDIN within 10 seconds.\n");
    } 
    else {
        // select() returned a number > 0, meaning a file descriptor is ready for reading
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            printf("\n[SUCCESS] Data is available to read on STDIN!\n");
            
            // To prove it works, let's actually read what was typed
            char buffer[1024];
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("You typed: %s", buffer);
            }
        }
    }

    return EXIT_SUCCESS;
}