#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGVTALRM\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    struct itimerval timer = {0};
    timer.it_value.tv_sec = 2;

    signal(SIGVTALRM, handler);

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    while (1) {
        /* Consume user CPU time. */
    }

    return 0;
}