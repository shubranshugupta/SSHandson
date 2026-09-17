#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    char msg_orig[]  = "1. Written using the ORIGINAL fd.\n";
    char msg_dup[]   = "2. Written using the DUP() fd.\n";
    char msg_dup2[]  = "3. Written using the DUP2() fd.\n";
    char msg_fcntl[] = "4. Written using the FCNTL() fd.\n";

    // 1. Open the file originally
    // O_APPEND ensures that every write automatically goes to the end of the file
    int fd_orig = open("file.txt", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0644);
    if (fd_orig == -1) {
        perror("Error opening file");
        return 1;
    }
    printf("Original FD assigned: %d\n", fd_orig);
    write(fd_orig, msg_orig, sizeof(msg_orig) - 1);

    // ==========================================
    // a. Duplicate using dup()
    // dup() automatically picks the lowest available FD number.
    // ==========================================
    int fd_dup = dup(fd_orig);
    if (fd_dup != -1) {
        printf("dup() FD assigned: %d\n", fd_dup);
        write(fd_dup, msg_dup, sizeof(msg_dup) - 1);
    }

    // ==========================================
    // b. Duplicate using dup2()
    // dup2(old_fd, new_fd) forces the OS to use a specific FD number you choose.
    // If FD 10 is already open, dup2 will safely close it first.
    // ==========================================
    int fd_dup2 = dup2(fd_orig, 10);
    if (fd_dup2 != -1) {
        printf("dup2() FD assigned: %d\n", fd_dup2);
        write(fd_dup2, msg_dup2, sizeof(msg_dup2) - 1);
    }

    // ==========================================
    // c. Duplicate using fcntl()
    // fcntl() with F_DUPFD finds the lowest available FD that is 
    // GREATER THAN OR EQUAL to the number you provide (10 in this case).
    // ==========================================
    int fd_fcntl = fcntl(fd_orig, F_DUPFD, 10);
    if (fd_fcntl != -1) {
        printf("fcntl() FD assigned: %d\n", fd_fcntl);
        write(fd_fcntl, msg_fcntl, sizeof(msg_fcntl) - 1);
    }
    
    // Close all file descriptors
    close(fd_orig);
    close(fd_dup);
    close(fd_dup2);
    close(fd_fcntl);

    // ==========================================
    // VERIFICATION STEP
    // Open the file again in Read-Only mode to check if it updated properly
    // ==========================================
    printf("\n--- Checking File Contents ---\n");
    
    int fd_check = open("file.txt", O_RDONLY);
    char buffer[1024];
    ssize_t bytes_read;
    
    // Read and print to STDOUT (FD 1)
    while ((bytes_read = read(fd_check, buffer, sizeof(buffer))) > 0) {
        write(1, buffer, bytes_read);
    }
    
    close(fd_check);

    return 0;
}