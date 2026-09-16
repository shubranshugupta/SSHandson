#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = creat("my_file.txt", 0644);
    
    if (fd == -1) {
        perror("Error creating file");
        return 1;
    }
    
    printf("File created successfully!\n");
    printf("The File Descriptor (FD) value is: %d\n", fd);
    
    close(fd);
    return 0;
}