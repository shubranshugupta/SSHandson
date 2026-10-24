#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void print_limit(const char *name, int parameter) {
    errno = 0;
    long value = sysconf(parameter);

    if (value != -1)
        printf("%s: %ld\n", name, value);
    else if (errno == 0)
        printf("%s: No definite limit\n", name);
    else
        perror(name);
}

int main(void) {
    print_limit("Maximum argument + environment size (bytes)", _SC_ARG_MAX);
    print_limit("Maximum processes per user ID", _SC_CHILD_MAX);
    print_limit("Clock ticks per second", _SC_CLK_TCK);
    print_limit("Maximum open files per process", _SC_OPEN_MAX);
    print_limit("Page size (bytes)", _SC_PAGESIZE);
    print_limit("Total physical memory pages", _SC_PHYS_PAGES);
    print_limit("Currently available physical memory pages", _SC_AVPHYS_PAGES);

    return 0;
}