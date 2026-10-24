#define _DEFAULT_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void) {
    if (mknod("fifo_syscall", S_IFIFO | 0666, 0) == -1) {
        perror("mknod");
        return 1;
    }

    printf("FIFO created using mknod()\n");
    return 0;
}