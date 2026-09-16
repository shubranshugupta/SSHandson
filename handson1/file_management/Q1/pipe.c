#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main() {
    // mkfifo(const char *pathname, mode_t mode);
    if (mkfifo("fifoPipePro", 0666) == 0) {
        printf("FIFO created successfully.\n");
    } else {
        perror("Error creating FIFO");
        return 1;
    }
    return 0;
}
