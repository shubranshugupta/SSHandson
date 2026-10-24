#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    char message[] = "Hello from parent!";
    char buffer[100];
    pid_t pid;

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid == 0) {
        /* Child reads from the pipe. */
        close(fd[1]);

        ssize_t count = read(fd[0], buffer, sizeof(buffer) - 1);

        if (count == -1) {
            perror("read");
            close(fd[0]);
            return 1;
        }

        buffer[count] = '\0';
        printf("Child received: %s\n", buffer);
        close(fd[0]);
    } else {
        /* Parent writes to the pipe. */
        close(fd[0]);

        if (write(fd[1], message, sizeof(message) - 1) == -1)
            perror("write");

        close(fd[1]);
        wait(NULL);
    }

    return 0;
}