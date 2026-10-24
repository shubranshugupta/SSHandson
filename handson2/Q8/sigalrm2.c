#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGALRM using ITIMER_REAL\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    struct itimerval timer = {0};
    timer.it_value.tv_sec = 2;

    signal(SIGALRM, handler);

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    while (1)
        pause();

    return 0;
}