#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void) {
    struct sigaction action = {0};
    sigemptyset(&action.sa_mask);

    /* Ignore SIGINT (Ctrl+C). */
    action.sa_handler = SIG_IGN;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("SIGINT ignored for 5 seconds. Try Ctrl+C.\n");
    sleep(5);

    /* Restore the default action: terminate the process. */
    action.sa_handler = SIG_DFL;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Default action restored. Press Ctrl+C to exit.\n");

    while (1)
        pause();

    return 0;
}