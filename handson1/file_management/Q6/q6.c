#include <unistd.h>

#define BUFFER_SIZE 1024

int main() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // read() takes 3 arguments:
    // 1. The File Descriptor (0 for STDIN)
    // 2. The memory buffer to store the data
    // 3. The maximum number of bytes to read at once
    
    // The loop continues as long as read() returns a number greater than 0.
    // If it returns 0, it means EOF (End of File). If -1, it means an error occurred.
    while ((bytes_read = read(0, buffer, BUFFER_SIZE)) > 0) {
        
        // write() takes 3 arguments:
        // 1. The File Descriptor (1 for STDOUT)
        // 2. The memory buffer containing the data
        // 3. The exact number of bytes to write (which is how many we just read)
        
        ssize_t bytes_written = write(1, buffer, bytes_read);
        
        // Basic error checking to ensure writing didn't fail
        if (bytes_written != bytes_read) {
            // Writing to STDERR (FD 2) if something goes wrong
            write(2, "Error writing data\n", 19);
            return 1;
        }
    }

    if (bytes_read == -1) {
        write(2, "Error reading data\n", 19);
        return 1;
    }

    return 0; // Exit successfully
}