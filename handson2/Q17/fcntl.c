#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(fd[0]);
        close(STDOUT_FILENO);

        if (fcntl(fd[1], F_DUPFD, STDOUT_FILENO) == -1) {
            perror("fcntl");
            _exit(1);
        }
        close(fd[1]);

        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp");
        _exit(1);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        wait(NULL);
        return 1;
    }

    if (pid == 0) {
        close(fd[1]);
        close(STDIN_FILENO);

        if (fcntl(fd[0], F_DUPFD, STDIN_FILENO) == -1) {
            perror("fcntl");
            _exit(1);
        }
        close(fd[0]);

        execlp("wc", "wc", (char *)NULL);
        perror("execlp");
        _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);
    wait(NULL);
    return 0;
}