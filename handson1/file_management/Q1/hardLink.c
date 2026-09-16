#include <unistd.h>
#include <stdio.h>

int main() {
    // link(const char *oldpath, const char *newpath);
    if (link("text1.txt", "hard_link_text1_pro.txt") == 0) {
        printf("Hard link created successfully.\n");
    } else {
        perror("Error creating hard link");
        return 1;
    }
    return 0;
}
