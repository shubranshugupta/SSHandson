#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    // 1. Ensure the user provided exactly two file names
    if (argc != 3) {
        // argv[0] is the name of the program itself (e.g., ./a.out)
        write(2, "Usage: ./a.out <source_file> <destination_file>\n", 48);
        return 1;
    }

    // 2. Open the Source File in Read-Only mode
    int fd_src = open(argv[1], O_RDONLY);
    if (fd_src == -1) {
        perror("Error opening source file");
        return 1;
    }

    // 3. Open the Destination File
    // O_WRONLY: Write mode
    // O_CREAT: Create it if it doesn't exist
    // O_TRUNC: If it DOES exist, wipe its contents completely before writing (like cp does)
    // 0644: Standard permissions (Read/Write for owner, Read for others)
    int fd_dest = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dest == -1) {
        perror("Error opening/creating destination file");
        close(fd_src); // Clean up the source FD before exiting
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    // 4. The Copy Loop
    // Read chunks of up to 4096 bytes until we hit EOF (read returns 0)
    while ((bytes_read = read(fd_src, buffer, BUFFER_SIZE)) > 0) {
        
        // Write exactly the number of bytes we just read
        bytes_written = write(fd_dest, buffer, bytes_read);
        
        if (bytes_written != bytes_read) {
            perror("Error writing data to destination");
            close(fd_src);
            close(fd_dest);
            return 1;
        }
    }

    if (bytes_read == -1) {
        perror("Error reading from source file");
    }

    // 5. Clean up
    close(fd_src);
    close(fd_dest);
    
    printf("Successfully copied '%s' to '%s'.\n", argv[1], argv[2]);

    return 0;
}