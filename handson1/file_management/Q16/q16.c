#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Ensure the user provides a filename and the lock type (r or w)
    if (argc != 3) {
        write(2, "Usage: ./a.out <filename> <r|w>\n", 33);
        return 1;
    }

    char *filename = argv[1];
    char lock_type = argv[2][0];

    // Open the file in Read/Write mode
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // Initialize the flock structure to zero
    struct flock fl;
    memset(&fl, 0, sizeof(fl));

    // a. Implement Write Lock (Exclusive Lock)
    // b. Implement Read Lock (Shared Lock)
    if (lock_type == 'w') {
        fl.l_type = F_WRLCK;  
        printf("Attempting to acquire a WRITE lock on '%s'...\n", filename);
    } else if (lock_type == 'r') {
        fl.l_type = F_RDLCK;  
        printf("Attempting to acquire a READ lock on '%s'...\n", filename);
    } else {
        printf("Invalid lock type. Use 'r' for read or 'w' for write.\n");
        close(fd);
        return 1;
    }

    // Configure the lock to cover the entire file
    fl.l_whence = SEEK_SET; // Start from the beginning of the file
    fl.l_start = 0;         // Offset is 0
    fl.l_len = 0;           // 0 means lock the whole file until EOF

    // Apply the lock. 
    // F_SETLKW means "Wait" (block) if another program already holds a conflicting lock.
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("Error applying lock");
        close(fd);
        return 1;
    }

    printf("Lock acquired successfully!\n");
    printf("Press ENTER to release the lock and exit...\n");
    
    // Pause the program so you can test the lock from another terminal
    getchar();

    // Release the lock
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("Error releasing lock");
    } else {
        printf("Lock released.\n");
    }

    close(fd);
    return 0;
}