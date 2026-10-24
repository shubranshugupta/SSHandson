#include <stdio.h>
#include <sys/resource.h>

int main(void) {
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return 1;
    }

    /* Keep the new soft limit within the existing hard limit. */
    limit.rlim_cur = (limit.rlim_max < 64) ? limit.rlim_max : 64;

    if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("setrlimit");
        return 1;
    }

    printf("Open file descriptor limit set to %llu\n", (unsigned long long)limit.rlim_cur);

    return 0;
}