#include <stdio.h>
#include <sys/resource.h>

void print_limit(char *name, int resource) {
    struct rlimit limit;

    if (getrlimit(resource, &limit) == -1) {
        perror(name);
        return;
    }

    printf("%-25s", name);

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("Soft: Unlimited\t");
    else
        printf("Soft: %llu\t", (unsigned long long)limit.rlim_cur);

    if (limit.rlim_max == RLIM_INFINITY)
        printf("Hard: Unlimited\n");
    else
        printf("Hard: %llu\n", (unsigned long long)limit.rlim_max);
}

int main(void) {
    print_limit("CPU time (seconds)", RLIMIT_CPU);
    print_limit("File size (bytes)", RLIMIT_FSIZE);
    print_limit("Data size (bytes)", RLIMIT_DATA);
    print_limit("Stack size (bytes)", RLIMIT_STACK);
    print_limit("Core file size (bytes)", RLIMIT_CORE);
    print_limit("Open files (count)", RLIMIT_NOFILE);
    print_limit("Address space (bytes)", RLIMIT_AS);

    return 0;
}