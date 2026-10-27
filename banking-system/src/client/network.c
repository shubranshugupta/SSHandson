/* The client collects complete requests locally, then sends them through this helper. */
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Display a prompt and read one bounded line from the keyboard.
 *
 * @param[in] prompt (const char *) Null-terminated prompt printed to standard output.
 * @param[out] text (char *) Writable buffer of size bytes; receives the input without CR/LF on
 *                  success.
 * @param[in] size (size_t) Buffer capacity in bytes, including the null terminator; use at
 *                 least 2.
 *
 * @return (int) 1 for a complete input line; 0 on end of input, read failure, or a line that
 *         does not fit.
 *
 * @note This function only reads input; callers validate the field contents. Password input is
 *       visible in the terminal.
 */
int get_text(const char *prompt, char *text, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(text, (int)size, stdin))
        return 0;
    if (!strchr(text, '\n'))
    {
        puts("Input too long or incomplete.");
        return 0;
    }
    text[strcspn(text, "\r\n")] = 0;
    return 1;
}

/**
 * @brief Keep prompting until the user enters an integer menu choice in range.
 *
 * @param[in] prompt (const char *) Null-terminated menu prompt.
 * @param[in] maximum (int) Inclusive largest permitted choice; zero is also permitted.
 *
 * @return (int) A choice from 0 through maximum, or -1 if keyboard input fails or ends.
 *
 * @note Zero represents exit or cancellation in the menus. Invalid numeric choices are
 *       requested again.
 */
int get_choice(const char *prompt, int maximum)
{
    char text[32];
    for (;;)
    {
        if (!get_text(prompt, text, sizeof(text)))
            return -1;
        int choice = parse_id(text);
        if (choice >= 0 && choice <= maximum)
            return choice;
        printf("Enter a number from 0 to %d.\n", maximum);
    }
}

/**
 * @brief Send one complete request and collect the server response up to its END marker.
 *
 * @param[in] client (Client *) Client connection and login information, including its connected
 *                   TCP socket; the socket remains owned by the caller.
 * @param[in] request (const char *) Null-terminated pipe-delimited request, shorter than
 *                    REQUEST_SIZE, without a newline.
 * @param[out] response (char *) Writable buffer of at least RESPONSE_SIZE bytes; receives
 *                      response lines excluding END.
 *
 * @return (int) 1 when the complete response is received; 0 for invalid request framing,
 *         transport failure, or response overflow.
 *
 * @note Appends the request newline and joins response lines with newlines. A return of 1 can
 *       still contain an ERROR reply from the server. Does not close the socket.
 */
int send_request(Client *client, const char *request, char *response)
{
    response[0] = 0;
    if (strlen(request) >= REQUEST_SIZE || strchr(request, '\n'))
        return 0;
    if (!send_all(client->fd, request) || !send_all(client->fd, "\n"))
        return 0;
    char line[4096];
    size_t used = 0;
    while (receive_line(client->fd, line, sizeof(line)))
    {
        if (!strcmp(line, "END"))
            return 1;
        size_t length = strlen(line);
        if (used + length + 2 >= RESPONSE_SIZE)
            return 0;
        memcpy(response + used, line, length);
        used += length;
        response[used++] = '\n';
        response[used] = 0;
    }
    return 0;
}

/**
 * @brief Print the server reply with a readable SUCCESS or ERROR prefix.
 *
 * @param[in] response (const char *) Null-terminated response text, normally obtained from
 *                     send_request().
 *
 * @return (void) No value. Writes the formatted response to standard output.
 */
void show_response(const char *response)
{
    if (!strncmp(response, "SUCCESS|", 8))
        printf("SUCCESS: %s", response + 8);
    else if (!strncmp(response, "ERROR|", 6))
        printf("ERROR: %s", response + 6);
    else
        printf("%s", response);
}

/**
 * @brief Send an operation request and display the received response.
 *
 * @param[in] client (Client *) Client connection and login information, including its connected
 *                   TCP socket; the socket remains owned by the caller.
 * @param[in] request (const char *) Complete null-terminated pipe-delimited operation request
 *                    without a newline.
 *
 * @return (int) 1 if a complete response was received and displayed, including an ERROR
 *         response; 0 if communication failed.
 *
 * @note Uses a local RESPONSE_SIZE buffer and leaves socket cleanup to the caller.
 */
int perform_request(Client *client, const char *request)
{
    char response[RESPONSE_SIZE];
    if (!send_request(client, request, response))
    {
        puts("Server connection closed.");
        return 0;
    }
    show_response(response);
    return 1;
}
