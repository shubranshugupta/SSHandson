#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void) {
    /* Ignore Ctrl+C for 5 seconds. */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("SIGINT ignored. Try pressing Ctrl+C for 5 seconds.\n");
    sleep(5);

    /* Restore the default action: terminate the process. */
    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("SIGINT default action restored. Press Ctrl+C to exit.\n");

    while (1)
        pause();

    return 0;
}