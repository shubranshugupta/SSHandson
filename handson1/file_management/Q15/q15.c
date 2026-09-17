#include <stdio.h>

extern char **environ;

int main() {
    // 1. Create a pointer to iterate through the environ array
    char **env_ptr = environ;

    printf("--- User Environment Variables ---\n\n");

    // 2. Loop through the array. 
    // The array is guaranteed to be terminated by a NULL pointer.
    while (*env_ptr != NULL) {
        // Print the current environment string (usually in KEY=VALUE format)
        printf("%s\n", *env_ptr);
        
        // Move the pointer to the next string in the array
        env_ptr++;
    }

    return 0;
}