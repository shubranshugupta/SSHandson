#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(void) {
    pid_t parent_pid = getpid();
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("Child PID: %ld\n", (long)getpid());
        printf("Parent PID before kill: %ld\n", (long)getppid());
        fflush(stdout);

        /* Child kills its parent. */
        if (kill(parent_pid, SIGKILL) == -1) {
            perror("kill");
            exit(1);
        }

        /* Wait until the child is adopted by another process. */
        while (getppid() == parent_pid)
            usleep(1000);

        printf("Parent PID after kill: %ld\n", (long)getppid());
        printf("Child is now an orphan process.\n");
    } else {
        while (1)
            pause();
    }

    return 0;
}