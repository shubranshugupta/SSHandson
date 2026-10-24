#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int p1[2], p2[2];

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

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close(p1[0]);
            close(p1[1]);
            close(p2[0]);
            close(p2[1]);

            while (wait(NULL) > 0)
                ;

            return 1;
        }

        if (pid == 0) {
            if (i == 0) {
                /* ls writes into the first pipe. */
                if (dup2(p1[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
            } else if (i == 1) {
                /* grep reads from p1 and writes into p2. */
                if (dup2(p1[0], STDIN_FILENO) == -1 ||
                    dup2(p2[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
            } else {
                /* wc reads from the second pipe. */
                if (dup2(p2[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
            }

            close(p1[0]);
            close(p1[1]);
            close(p2[0]);
            close(p2[1]);

            if (i == 0)
                execlp("ls", "ls", "-l", (char *)NULL);
            else if (i == 1)
                execlp("grep", "grep", "^d", (char *)NULL);
            else
                execlp("wc", "wc", "-l", (char *)NULL);

            perror("execlp");
            _exit(1);
        }
    }

    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);

    for (int i = 0; i < 3; i++)
        wait(NULL);

    return 0;
}