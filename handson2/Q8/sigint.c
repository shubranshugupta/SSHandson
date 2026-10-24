#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "\nCaught SIGINT\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    signal(SIGINT, handler);

    while (1)
        pause();

    return 0;
}