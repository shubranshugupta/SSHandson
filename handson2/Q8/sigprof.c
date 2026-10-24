#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGPROF\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    struct itimerval timer = {0};
    timer.it_value.tv_sec = 2;

    signal(SIGPROF, handler);

    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    while (1) {
        /* Consume CPU time. */
    }

    return 0;
}