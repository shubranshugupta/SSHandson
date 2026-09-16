#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    // 10 bytes of data for the first write
    char buf1[] = "1234567890";
    // 10 bytes of data for the second write
    char buf2[] = "ABCDEFGHIJ";

    // 1. Open the file in Read/Write mode. 
    // Create it if it doesn't exist, and truncate it to 0 if it does.
    int fd = open("file.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // 2. Write the first 10 bytes
    if (write(fd, buf1, 10) != 10) {
        perror("Error writing first buffer");
        close(fd);
        return 1;
    }

    // 3. Move the file pointer by 10 bytes using lseek
    // SEEK_CUR means "start from the CURrent position of the pointer"
    off_t offset = lseek(fd, 10, SEEK_CUR);

    // a. Check the return value of lseek
    if (offset == (off_t)-1) {
        perror("Error during lseek");
        close(fd);
        return 1;
    } else {
        // The return value of lseek is the new offset measured from the beginning of the file.
        // It should be 20 (10 bytes written + 10 bytes skipped).
        printf("Success: lseek() returned a new offset of: %lld bytes from the start of the file.\n", (long long)offset);
    }

    // 4. Write the second 10 bytes
    if (write(fd, buf2, 10) != 10) {
        perror("Error writing second buffer");
        close(fd);
        return 1;
    }

    // 5. Close the file
    close(fd);
    
    printf("File 'file.txt' created successfully. Use 'od' command to inspect it.\n");

    return 0;
}