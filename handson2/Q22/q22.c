#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>

int main(void) {
    const char *fifo = "myfifo";
    char buffer[100];

    if (mkfifo(fifo, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    int fd = open(fifo, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* Keep a writer open to prevent EOF when no external writer exists. */
    int keep_open = open(fifo, O_WRONLY | O_NONBLOCK);
    if (keep_open == -1) {
        perror("open writer");
        close(fd);
        return 1;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval timeout = {10, 0};

    printf("Waiting for data for 10 seconds...\n");
    fflush(stdout);

    int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
    int status = 0;

    if (result == -1) {
        perror("select");
        status = 1;
    } else if (result == 0) {
        printf("Timeout: no data received within 10 seconds.\n");
    } else {
        ssize_t count = read(fd, buffer, sizeof(buffer) - 1);

        if (count == -1) {
            perror("read");
            status = 1;
        } else {
            buffer[count] = '\0';
            printf("Received: %s\n", buffer);
        }
    }

    close(keep_open);
    close(fd);
    return status;
}