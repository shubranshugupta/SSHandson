#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void) {
    if (mkfifo("fifo_library", 0666) == -1) {
        perror("mkfifo");
        return 1;
    }

    printf("FIFO created using mkfifo()\n");
    return 0;
}