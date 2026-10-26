#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void) {
    int server_fd, client_fd;
    char buffer[100];
    char reply[] = "Hello from server!";

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Waiting for a client on port 8080...\n");

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    /* Read until the client closes its sending direction. */
    ssize_t count;
    printf("Client: ");
    while ((count = recv(client_fd, buffer, sizeof(buffer), 0)) > 0)
        fwrite(buffer, 1, (size_t)count, stdout);
    printf("\n");

    if (count == -1) {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    /* TCP may send fewer bytes than requested. */
    size_t sent = 0;
    while (sent < sizeof(reply) - 1) {
        count = send(client_fd, reply + sent, sizeof(reply) - 1 - sent, MSG_NOSIGNAL);
        if (count <= 0) {
            perror("send");
            close(client_fd);
            close(server_fd);
            return 1;
        }
        sent += (size_t)count;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}