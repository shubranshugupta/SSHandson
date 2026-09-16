#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main() {
    int setup_fd = creat("existing_file.txt", 0644);
    close(setup_fd);

    int fd_rw = open("existing_file.txt", O_RDWR);
    if (fd_rw != -1) {
        printf("Success: Opened existing file in O_RDWR mode. FD: %d\n", fd_rw);
        close(fd_rw);
    } else {
        perror("Failed to open file in O_RDWR mode");
    }

    int fd_excl = open("existing_file.txt", O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd_excl == -1) {
        perror("Expected Error (O_EXCL blocked it because file exists)");
    } else {
        printf("Opened with O_EXCL. FD: %d\n", fd_excl);
        close(fd_excl);
    }

    int fd_excl1 = open("existing_file1.txt", O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd_excl1 == -1) {
        perror("Expected Error (O_EXCL blocked it because file exists)");
    } else {
        printf("Opened with O_EXCL. FD: %d\n", fd_excl1);
        close(fd_excl1);
    }
    remove("existing_file1.txt");

    return 0;
}