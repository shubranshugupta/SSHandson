#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
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

        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2");
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