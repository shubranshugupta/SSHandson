#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGFPE\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    struct sigaction action = {0};

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGFPE, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    raise(SIGFPE);
    return 0;
}