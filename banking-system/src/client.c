/* Main client login loop. All prompts are local; the server receives complete requests. */
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
 * @brief Create an IPv4 TCP socket and connect it to the banking server.
 *
 * @param[in] ip (const char *) Null-terminated numeric IPv4 server address, for example
 *               "127.0.0.1".
 * @param[in] port (int) Server TCP port from 1 through 65535.
 *
 * @return (int) The connected socket descriptor, which may be zero, or -1 if socket creation,
 *         address conversion, or connection fails.
 *
 * @note Closes a partially created socket on failure. The caller owns a successful socket and
 *       must close it.
 */
static int connect_to_server(const char *ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1)
    {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("connect");
        close(fd);
        return -1;
    }
    return fd;
}

/**
 * @brief Start the client, display the login-role menu, authenticate, and call the selected
 *        role menu.
 *
 * @param[in] argc (int) Command-line argument count; must be 3 including the executable name.
 * @param[in] argv (char **) Argument vector: argv[1] is the server IPv4 address and argv[2] is
 *                 its TCP port.
 *
 * @return (int) 0 when the main menu exits or keyboard input ends; 1 for invalid command-line
 *         arguments or port.
 *
 * @note Creates a connection for each login attempt and closes it after the role menu returns.
 *       Password and request buffers are cleared after the login request.
 */
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s SERVER_IP PORT\n", argv[0]);
        return 1;
    }
    int port = parse_id(argv[2]);
    if (port < 1 || port > 65535)
        return 1;
    const int roles[] = {0, CUSTOMER, ADMIN, MANAGER, EMPLOYEE};
    while (1)
    {
        puts("\nBANK LOGIN MENU\n1 Customer\n2 Administrator\n3 Manager\n4 Employee\n0 Exit");
        int choice = get_choice("Login as: ", 4);
        if (choice <= 0)
            break;
        char username[64], password[128];
        if (!get_text("Username: ", username, sizeof(username)) ||
            !get_text("Password: ", password, sizeof(password)))
            break;
        if (!valid_text(username, 1, 32) || !valid_text(password, 1, 64))
        {
            puts("Invalid credentials format.");
            continue;
        }
        Client client = {0};
        client.fd = connect_to_server(argv[1], port);
        if (client.fd == -1)
        {
            puts("Cannot connect to server.");
            continue;
        }
        char request[REQUEST_SIZE], response[RESPONSE_SIZE];
        snprintf(request, sizeof(request), "LOGIN|%d|%s|%s", roles[choice], username, password);
        memset(password, 0, sizeof(password));
        int received = send_request(&client, request, response);
        memset(request, 0, sizeof(request));
        if (received &&
            sscanf(response, "SUCCESS|%d|%d|%d", &client.user_id, &client.role,
                   &client.account_id) == 3 &&
            client.role == roles[choice])
        {
            printf("Logged in as %s.\n", role_name(client.role));
            switch (client.role)
            {
            case CUSTOMER:
                customer_menu(&client);
                break;
            case EMPLOYEE:
                employee_menu(&client);
                break;
            case MANAGER:
                manager_menu(&client);
                break;
            case ADMIN:
                admin_menu(&client);
                break;
            }
        }
        else if (received)
            show_response(response);
        else
            puts("Login connection failed.");
        close(client.fd);
    }
    return 0;
}
