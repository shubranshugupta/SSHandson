#include <stdio.h>
#include <unistd.h>

int main(void) {
    int fd[2];
    char buffer[100];
    char message[] = "Hello from the pipe!";
    ssize_t count;

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    /* fd[1] is the write end. */
    if (write(fd[1], message, sizeof(message) - 1) == -1) {
        perror("write");
        return 1;
    }
    close(fd[1]);

    /* fd[0] is the read end. */
    count = read(fd[0], buffer, sizeof(buffer) - 1);

    if (count == -1) {
        perror("read");
        close(fd[0]);
        return 1;
    }

    buffer[count] = '\0';
    printf("Message read from pipe: %s\n", buffer);

    close(fd[0]);
    return 0;
}