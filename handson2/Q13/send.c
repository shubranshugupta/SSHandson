#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <receiver_pid>\n", argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]);

    if (pid <= 0) {
        fprintf(stderr, "Enter a positive PID.\n");
        return 1;
    }

    if (kill((pid_t)pid, SIGSTOP) == -1) {
        perror("kill");
        return 1;
    }

    printf("SIGSTOP sent to process %d\n", pid);
    return 0;
}