#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGSTOP\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void) {
    printf("Receiver PID: %ld\n", (long)getpid());

    if (signal(SIGSTOP, handler) == SIG_ERR)
        perror("Cannot register SIGSTOP handler");

    printf("Waiting for signals...\n");
    fflush(stdout);

    while (1)
        pause();

    return 0;
}