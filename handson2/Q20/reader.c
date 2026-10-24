#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int main(void) {
    const char *fifo = "/tmp/demo_fifo";
    char buffer[100];
    ssize_t count;

    if (mkfifo(fifo, 0600) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    int fd = open(fifo, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    while ((count = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        printf("%s", buffer);
    }

    if (count == -1)
        perror("read");
    else
        printf("\n");

    close(fd);
    unlink(fifo);

    return count == -1 ? 1 : 0;
}