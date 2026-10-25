#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int main(void) {
    char buffer[100];
    char message[] = "Hello from Program 1!";
    ssize_t count;

    if (mkfifo("fifo1", 0600) == -1 && errno != EEXIST) {
        perror("mkfifo fifo1");
        return 1;
    }

    if (mkfifo("fifo2", 0600) == -1 && errno != EEXIST) {
        perror("mkfifo fifo2");
        return 1;
    }

    /* Send message. */
    int fd = open("fifo1", O_WRONLY);
    if (fd == -1) {
        perror("open fifo1");
        return 1;
    }

    if (write(fd, message, sizeof(message) - 1) == -1) {
        perror("write");
        close(fd);
        return 1;
    }
    close(fd);

    /* Receive reply. */
    fd = open("fifo2", O_RDONLY);
    if (fd == -1) {
        perror("open fifo2");
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
    unlink("fifo1");
    unlink("fifo2");

    return count == -1 ? 1 : 0;
}