#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

volatile sig_atomic_t expired = 0;

void handler(int sig) {
    expired = 1;
}

int main(void) {
    struct itimerval timer;
    struct timeval interval = {10, 0}; /* seconds, microseconds */

    signal(SIGVTALRM, handler);

    timer.it_value = interval;
    timer.it_interval = interval;

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    while (1) {
        if (expired) {
            expired = 0;
            printf("ITIMER_VIRTUAL expired\n");
        }
    }
}