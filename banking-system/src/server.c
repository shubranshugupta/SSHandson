/* Thread-per-client server. Requests contain all fields; the server never asks keyboard questions.
 */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
static int client_count;

/**
 * @brief Authenticate or route a complete request to the handler for the logged-in role.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] v (char **) Array of n null-terminated request fields; v[0] is the action name.
 * @param[in] n (int) Number of elements in v, including the action name.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 when LOGOUT or a failed login-ownership check requires connection closure; 0
 *         for other requests, whether successful or unsuccessful.
 *
 * @note Checks exclusive login ownership and refreshes the active user before role operations.
 *       The return value controls connection closure; reply describes the operation result.
 */
static int dispatch(Session *s, char **v, int n, char *reply)
{
    if (n < 1)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid request.");
        return 0;
    }
    if (!strcmp(v[0], "LOGIN"))
    {
        handle_login(s, v, n, reply);
        return 0;
    }
    if (!s->user_id)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Login required.");
        return 0;
    }
    if (!strcmp(v[0], "LOGOUT") && n == 1)
    {
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Logged out.");
        return 1;
    }
    if (!check_login(s, reply))
        return 1;
    if (!current_user(s, 0, reply))
        return 0;
    if (!strcmp(v[0], "CHANGE_PASS") && n == 2)
    {
        change_password(s, v[1], reply);
        return 0;
    }
    int handled = 0;
    switch (s->role)
    {
    case CUSTOMER:
        handled = handle_customer_action(s, v, n, reply);
        break;
    case EMPLOYEE:
        handled = handle_employee_action(s, v, n, reply);
        break;
    case MANAGER:
        handled = handle_manager_action(s, v, n, reply);
        break;
    case ADMIN:
        handled = handle_admin_action(s, v, n, reply);
        break;
    }
    if (!handled)
        snprintf(reply, RESPONSE_SIZE, "ERROR|Action not allowed for this role or invalid fields.");
    return 0;
}

/**
 * @brief Serve one connected client in a detached pthread with a private database connection.
 *
 * @param[in] argument (void *) Pointer to a heap-allocated int containing the accepted socket
 *                     descriptor; ownership transfers to this function.
 *
 * @return (void *) NULL when the client disconnects, logs out, or an initialization or
 *         communication failure ends the thread.
 *
 * @note Frees argument, initializes MySQL thread state, reads and dispatches requests, and
 *       sends replies terminated by END.
 *
 * @note Cancels unfinished transactions and closes the session to release login ownership
 *       before logout acknowledgement or worker exit. Also releases the socket and client-count
 *       slot; committed transactions remain committed.
 */
static void *serve_client(void *argument)
{
    int fd = *(int *)argument;
    free(argument);
    if (mysql_thread_init())
        goto done;
    Session session = {0};
    session.db = open_database();
    if (!session.db)
    {
        send_all(fd, "ERROR|Database unavailable.\nEND\n");
        goto thread_done;
    }
    char request[REQUEST_SIZE], reply[RESPONSE_SIZE];
    while (receive_line(fd, request, sizeof(request)))
    {
        char *fields[10];
        int count = split_fields(request, fields, 10);
        reply[0] = 0;
        int logout = 0;
        if (count < 1)
            snprintf(reply, sizeof(reply), "ERROR|Too many fields.");
        else
            logout = dispatch(&session, fields, count, reply);
        /* All successful financial handlers commit; any unfinished transaction is cancelled. */
        if (session.db)
            cancel_transaction(&session);
        if (logout || !session.db)
        {
            /* Release login ownership before sending the logout or terminal-error reply. */
            close_session(&session);
            logout = 1;
        }
        if (!reply[0])
            snprintf(reply, sizeof(reply), "ERROR|Request did not produce a result.");
        if (!send_all(fd, reply) || !send_all(fd, "\nEND\n"))
            break;
        memset(request, 0, sizeof(request));
        if (logout)
            break;
    }
    if (session.db)
        cancel_transaction(&session);
    close_session(&session);
thread_done:
    mysql_thread_end();
done:
    close(fd);
    pthread_mutex_lock(&count_lock);
    client_count--;
    pthread_mutex_unlock(&count_lock);
    return NULL;
}

/**
 * @brief Initialize MySQL and the listening socket, then create a detached thread for each
 *        accepted client.
 *
 * @param[in] argc (int) Command-line argument count, including the executable name.
 * @param[in] argv (char **) Argument vector; optional argv[1] is the bind IPv4 address and
 *                 argv[2] is the TCP port, defaulting to 127.0.0.1 and 8080.
 *
 * @return (int) 1 on startup failure. After successful startup, the accept loop runs
 *         indefinitely.
 *
 * @note Allows at most 64 client threads. Each thread receives its own heap-allocated socket
 *       argument. Client sockets have no application-imposed send/receive timeout.
 */
int main(int argc, char **argv)
{
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? parse_id(argv[2]) : 8080;
    if (port < 1 || port > 65535)
    {
        fprintf(stderr, "Invalid port.\n");
        return 1;
    }
    if (mysql_library_init(0, NULL, NULL))
        return 1;
    MYSQL *check = open_database();
    if (!check)
        return 1;
    mysql_close(check);
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1)
    {
        perror("socket");
        return 1;
    }
    int option = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1)
    {
        close(listener);
        return 1;
    }
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) == -1 ||
        listen(listener, 64) == -1)
    {
        perror("bind/listen");
        close(listener);
        return 1;
    }
    printf("Bank server listening on %s:%d\n", ip, port);
    fflush(stdout);
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
        return 1;
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
        return 1;
    for (;;)
    {
        int fd = accept(listener, NULL, NULL);
        if (fd == -1)
        {
            if (errno != EINTR)
                perror("accept");
            continue;
        }
        pthread_mutex_lock(&count_lock);
        if (client_count >= 64)
        {
            pthread_mutex_unlock(&count_lock);
            close(fd);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&count_lock);
        int *argument = malloc(sizeof(*argument));
        int error = 1;
        if (argument)
        {
            *argument = fd;
            pthread_t thread;
            error = pthread_create(&thread, &attr, serve_client, argument);
        }
        if (error)
        {
            free(argument);
            close(fd);
            pthread_mutex_lock(&count_lock);
            client_count--;
            pthread_mutex_unlock(&count_lock);
        }
    }
}
