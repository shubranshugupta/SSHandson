#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd;
    pid_t pid;

    // 1. Open the file BEFORE calling fork()
    fd = open("file.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // 2. Fork the process
    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        close(fd);
        return 1;
    } 
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        char child_msg[] = "1. This line was written by the CHILD process.\n";
        
        // Write to the shared file descriptor
        write(fd, child_msg, strlen(child_msg));
        
        // Close the child's copy of the file descriptor
        close(fd);
    } 
    else {
        // --- PARENT PROCESS ---
        char parent_msg[] = "2. This line was written by the PARENT process.\n";
        
        // Write to the shared file descriptor
        write(fd, parent_msg, strlen(parent_msg));
        
        // Wait for the child to finish executing so the prompt doesn't return too early
        wait(NULL);
        
        // Close the parent's copy of the file descriptor
        close(fd);
        
        printf("Both processes finished writing to 'file.txt'.\n");
    }

    return 0;
}