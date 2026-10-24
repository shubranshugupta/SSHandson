#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    const char msg[] = "Caught SIGSEGV\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void) {
    signal(SIGSEGV, handler);
    raise(SIGSEGV);
    return 0;
}