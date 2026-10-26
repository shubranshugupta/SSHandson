#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

void handle_client(int fd) {
    char buffer[1024];
    ssize_t count;

    while ((count = recv(fd, buffer, sizeof(buffer), 0)) > 0) {
        ssize_t sent = 0;

        while (sent < count) {
            ssize_t n = send(fd, buffer + sent,
                             count - sent, MSG_NOSIGNAL);
            if (n <= 0)
                return;

            sent += n;
        }
    }
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &option, sizeof(option));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    /* Automatically reap child processes on Linux. */
    signal(SIGCHLD, SIG_IGN);

    printf("Fork server listening on port 8080...\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
            close(client_fd);
            _exit(0);
        }

        close(client_fd);
    }
}