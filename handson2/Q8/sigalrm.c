#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGALRM using alarm()\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    signal(SIGALRM, handler);
    alarm(2);

    while (1)
        pause();

    return 0;
}