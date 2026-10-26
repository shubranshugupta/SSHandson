#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_IP>\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    if (inet_pton(AF_INET, argv[1], &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address\n");
        close(fd);
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    char message[] = "Hello from client!";
    size_t sent = 0;
    ssize_t count;

    while (sent < sizeof(message) - 1) {
        count = send(fd, message + sent, sizeof(message) - 1 - sent, MSG_NOSIGNAL);
        if (count <= 0) {
            perror("send");
            close(fd);
            return 1;
        }
        sent += (size_t)count;
    }

    /* Signal that the complete request has been sent. */
    if (shutdown(fd, SHUT_WR) == -1) {
        perror("shutdown");
        close(fd);
        return 1;
    }

    char buffer[100];
    printf("Server: ");
    while ((count = recv(fd, buffer, sizeof(buffer), 0)) > 0)
        fwrite(buffer, 1, (size_t)count, stdout);
    printf("\n");

    if (count == -1)
        perror("recv");

    close(fd);
    return count == -1 ? 1 : 0;
}