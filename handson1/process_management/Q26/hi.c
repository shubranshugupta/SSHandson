#include <stdio.h>

int main(int argc, char *argv[]) {
    // Check if the input argument was passed
    if (argc != 2) {
        printf("Error: No name provided.\n");
        return 1;
    }
    
    // Print the greeting using the passed argument
    printf("--> Target Executable: Hello, %s! The execution was successful.\n", argv[1]);
    
    return 0;
}