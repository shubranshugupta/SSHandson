#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    // Open file in Write mode, create if it doesn't exist, wipe it if it does
    int fd = open("ticket.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error creating file");
        return 1;
    }

    // Set the starting ticket number to 1000
    char buffer[] = "1000\n";
    
    // Write it to the file using system call
    write(fd, buffer, strlen(buffer));
    
    printf("Successfully initialized 'ticket.txt' with starting number: 1000\n");
    
    close(fd);
    return 0;
}