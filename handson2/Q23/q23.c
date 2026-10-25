#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int main(void) {
    int fd[2];

    long max_files = sysconf(_SC_OPEN_MAX);
    if (max_files == -1) {
        fprintf(stderr, "Could not determine open-file limit\n");
        return 1;
    }

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    int pipe_size = fcntl(fd[0], F_GETPIPE_SZ);
    if (pipe_size == -1) {
        perror("fcntl");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    printf("Maximum open file descriptors per process: %ld\n",
           max_files);
    printf("Pipe buffer capacity: %d bytes\n", pipe_size);

    close(fd[0]);
    close(fd[1]);
    return 0;
}