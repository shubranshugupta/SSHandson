#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./a.out <a|b|c|d|e>\n");
        printf("a = execl, b = execlp, c = execle, d = execv, e = execvp\n");
        return 1;
    }

    char choice = argv[1][0];

    // Array (Vector) used for 'v' variants
    // The array MUST end with a NULL pointer
    char *args[] = {"ls", "-Rl", NULL};

    // Custom environment array used for 'e' variants
    char *custom_env[] = {"TEST_VAR=HelloWorld", "PATH=/bin:/usr/bin", NULL};

    printf("Executing 'ls -Rl' using variant '%c'...\n\n", choice);

    switch (choice) {
        case 'a':
            // execl: Absolute path + List of arguments
            execl("/bin/ls", "ls", "-Rl", NULL);
            perror("execl failed");
            break;

        case 'b':
            // execlp: Searches $PATH + List of arguments
            // We only need to provide "ls", not "/bin/ls"
            execlp("ls", "ls", "-Rl", NULL);
            perror("execlp failed");
            break;

        case 'c':
            // execle: Absolute path + List of args + Custom Environment
            execle("/bin/ls", "ls", "-Rl", NULL, custom_env);
            perror("execle failed");
            break;

        case 'd':
            // execv: Absolute path + Vector (Array) of arguments
            execv("/bin/ls", args);
            perror("execv failed");
            break;

        case 'e':
            // execvp: Searches $PATH + Vector (Array) of arguments
            execvp("ls", args);
            perror("execvp failed");
            break;

        default:
            printf("Invalid choice. Please pass a, b, c, d, or e.\n");
    }

    // This return is only reached if the exec call fails!
    return 1;
}