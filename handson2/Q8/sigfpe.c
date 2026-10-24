#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGFPE\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    signal(SIGFPE, handler);
    raise(SIGFPE);
    return 0;
}