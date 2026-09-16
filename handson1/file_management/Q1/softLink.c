#include <unistd.h>
#include <stdio.h>

int main() {
    // symlink(const char *target, const char *linkpath);
    if (symlink("text1.txt", "soft_link_text1_pro.txt") == 0) {
        printf("Soft link created successfully.\n");
    } else {
        perror("Error creating soft link");
        return 1;
    }
    return 0;
}
