#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int p1[2], p2[2];
    char buffer[100];
    ssize_t count;
    pid_t pid;

    if (pipe(p1) == -1) {
        perror("pipe");
        return 1;
    }

    if (pipe(p2) == -1) {
        perror("pipe");
        close(p1[0]);
        close(p1[1]);
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child: receive from parent, then reply. */
        close(p1[1]);
        close(p2[0]);

        count = read(p1[0], buffer, sizeof(buffer) - 1);
        if (count == -1) {
            perror("read");
            return 1;
        }

        buffer[count] = '\0';
        printf("Child received: %s\n", buffer);
        fflush(stdout);
        close(p1[0]);

        char message[] = "Hello from child!";
        if (write(p2[1], message, sizeof(message) - 1) == -1) {
            perror("write");
            return 1;
        }

        close(p2[1]);
    } else {
        /* Parent: send to child, then receive its reply. */
        close(p1[0]);
        close(p2[1]);

        char message[] = "Hello from parent!";
        if (write(p1[1], message, sizeof(message) - 1) == -1) {
            perror("write");
            return 1;
        }

        close(p1[1]);

        count = read(p2[0], buffer, sizeof(buffer) - 1);
        if (count == -1) {
            perror("read");
            return 1;
        }

        buffer[count] = '\0';
        printf("Parent received: %s\n", buffer);

        close(p2[0]);
        wait(NULL);
    }

    return 0;
}