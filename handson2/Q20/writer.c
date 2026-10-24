#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int main(void) {
    const char *fifo = "/tmp/demo_fifo";
    char message[] = "Hello from writer!";

    if (mkfifo(fifo, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    int fd = open(fifo, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (write(fd, message, sizeof(message) - 1) == -1) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}