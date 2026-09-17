#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    // 1. Open a file with a specific set of modes (Read/Write + Append)
    // We will use fcntl to "discover" these modes dynamically.
    int fd = open("test.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // 2. Use fcntl with F_GETFL to retrieve the file status flags
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        perror("Error using fcntl");
        close(fd);
        return 1;
    }

    // 3. Extract the Access Mode using the O_ACCMODE mask
    int access_mode = flags & O_ACCMODE;
    printf("--- File Access Mode ---\n");
    if (access_mode == O_RDONLY) {
        printf("Mode: READ-ONLY\n");
    } else if (access_mode == O_WRONLY) {
        printf("Mode: WRITE-ONLY\n");
    } else if (access_mode == O_RDWR) {
        printf("Mode: READ-WRITE\n");
    } else {
        printf("Mode: UNKNOWN\n");
    }

    // 4. Check for additional File Status Flags
    // Unlike access modes, things like O_APPEND or O_NONBLOCK *are* 
    // individual bits, so we check them directly with a bitwise AND.
    int additional_flag = flags & ~(O_ACCMODE);
    printf("\n--- Additional Status Flags ---\n");
    if (additional_flag & O_APPEND) {
        printf("Flag: APPEND is set.\n");
    }
    if (additional_flag & O_NONBLOCK) {
        printf("Flag: NON-BLOCKING is set.\n");
    }
    if (additional_flag & O_SYNC) {
        printf("Flag: SYNCHRONOUS WRITES is set.\n");
    }

    close(fd);
    return 0;
}